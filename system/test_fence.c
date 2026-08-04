/*
 * test_fence.c - Test LFENCE, MFENCE, and SFENCE instructions
 *
 * LFENCE: Serializes load operations (load fence). Ensures all prior loads
 *         are globally visible before any subsequent load executes.
 * SFENCE: Serializes store operations (store fence). Ensures all prior stores
 *         are globally visible before any subsequent store executes.
 * MFENCE: Full memory fence. Serializes all load and store operations.
 *
 * These instructions do not modify registers or flags. Testing is primarily
 * about verifying they execute without faulting and that memory ordering
 * is preserved.
 *
 * Prerequisites:
 *   LFENCE/MFENCE: SSE2 (universal on x86-64)
 *   SFENCE: SSE (universal on x86-64)
 *
 * Compile: gcc -o test_fence system/test_fence.c -O0
 * Note: Do not use static linking (-static)
 */

#define _GNU_SOURCE
#include "../common.h"
#include <pthread.h>

/* Test LFENCE executes without fault */
static void test_lfence_basic(void)
{
    TEST_START("LFENCE - Basic execution");

    uint64_t value = UINT64_C(0x0123456789abcdef);
    __asm__ volatile ("lfence" : "+r"(value) : : "memory");
    TEST_ASSERT(value == UINT64_C(0x0123456789abcdef), "LFENCE preserves registers");
}

/* Test SFENCE executes without fault */
static void test_sfence_basic(void)
{
    TEST_START("SFENCE - Basic execution");

    uint64_t value = UINT64_C(0xfedcba9876543210);
    __asm__ volatile ("sfence" : "+r"(value) : : "memory");
    TEST_ASSERT(value == UINT64_C(0xfedcba9876543210), "SFENCE preserves registers");
}

/* Test MFENCE executes without fault */
static void test_mfence_basic(void)
{
    TEST_START("MFENCE - Basic execution");

    uint64_t value = UINT64_C(0x8000000000000001);
    __asm__ volatile ("mfence" : "+r"(value) : : "memory");
    TEST_ASSERT(value == UINT64_C(0x8000000000000001), "MFENCE preserves registers");
}

/* Test multiple fences in sequence */
static void test_fence_sequence(void)
{
    TEST_START("FENCE - Multiple fences in sequence");

    __asm__ volatile (
        "sfence\n\t"
        "lfence\n\t"
        "mfence\n\t"
        "sfence\n\t"
        "lfence\n\t"
        "mfence"
    );

    uint64_t flags = get_flags();
    TEST_ASSERT(flags & (1ULL << 1), "Fence sequence returns with fixed RFLAGS bit 1 set");
}

/* Test LFENCE does not modify flags */
static void test_lfence_no_flag_change(void)
{
    uint64_t flags_before, flags_after;

    TEST_START("LFENCE - Does not modify flags");

    flags_before = get_flags();
    __asm__ volatile ("lfence");
    flags_after = get_flags();

    TEST_ASSERT(flags_before == flags_after,
                "LFENCE should not change flags: before=0x%" PRIX64 ", after=0x%" PRIX64,
                flags_before, flags_after);
}

/* Test SFENCE does not modify flags */
static void test_sfence_no_flag_change(void)
{
    uint64_t flags_before, flags_after;

    TEST_START("SFENCE - Does not modify flags");

    flags_before = get_flags();
    __asm__ volatile ("sfence");
    flags_after = get_flags();

    TEST_ASSERT(flags_before == flags_after,
                "SFENCE should not change flags: before=0x%" PRIX64 ", after=0x%" PRIX64,
                flags_before, flags_after);
}

/* Test MFENCE does not modify flags */
static void test_mfence_no_flag_change(void)
{
    uint64_t flags_before, flags_after;

    TEST_START("MFENCE - Does not modify flags");

    flags_before = get_flags();
    __asm__ volatile ("mfence");
    flags_after = get_flags();

    TEST_ASSERT(flags_before == flags_after,
                "MFENCE should not change flags: before=0x%" PRIX64 ", after=0x%" PRIX64,
                flags_before, flags_after);
}

/* Test SFENCE ensures store ordering */
static void test_sfence_store_ordering(void)
{
    volatile uint64_t a = 0, b = 0;

    TEST_START("SFENCE - Store ordering");

    /* Write a, then SFENCE, then write b. After SFENCE, store to a is complete. */
    __asm__ volatile (
        "movq $1, %0\n\t"
        "sfence\n\t"
        "movq $2, %1"
        : "=m"(a), "=m"(b)
        :
        : "memory"
    );

    /* Both stores should be visible */
    TEST_ASSERT(a == 1, "Store to a should be visible: expected 1, got %" PRIu64, a);
    TEST_ASSERT(b == 2, "Store to b should be visible: expected 2, got %" PRIu64, b);
}

/* Test LFENCE ensures load ordering */
static void test_lfence_load_ordering(void)
{
    volatile uint64_t x = 42;
    uint64_t val1, val2;

    TEST_START("LFENCE - Load ordering");

    __asm__ volatile (
        "movq %2, %0\n\t"
        "lfence\n\t"
        "movq %2, %1"
        : "=r"(val1), "=r"(val2)
        : "m"(x)
        : "memory"
    );

    TEST_ASSERT(val1 == 42, "First load should read 42, got %" PRIu64, val1);
    TEST_ASSERT(val2 == 42, "Second load should read 42, got %" PRIu64, val2);
}

/* Test MFENCE between store and load */
static void test_mfence_store_load(void)
{
    volatile uint64_t shared = 0;
    uint64_t result;

    TEST_START("MFENCE - Store then load ordering");

    __asm__ volatile (
        "movq $99, %1\n\t"
        "mfence\n\t"
        "movq %1, %0"
        : "=r"(result), "=m"(shared)
        :
        : "memory"
    );

    TEST_ASSERT(result == 99, "Load after MFENCE should see store: expected 99, got %" PRIu64, result);
}

enum { MFENCE_LITMUS_ITERATIONS = 10000 };

struct mfence_litmus {
    volatile uint32_t x;
    volatile uint32_t y;
    uint32_t read0;
    uint32_t read1;
    pthread_barrier_t start;
    pthread_barrier_t done;
};

struct mfence_worker_arg {
    struct mfence_litmus *state;
    int id;
};

static void *mfence_worker(void *opaque)
{
    struct mfence_worker_arg *arg = opaque;
    struct mfence_litmus *state = arg->state;
    for (int iteration = 0; iteration < MFENCE_LITMUS_ITERATIONS; iteration++) {
        pthread_barrier_wait(&state->start);
        if (arg->id == 0) {
            uint32_t value;
            __asm__ volatile (
                "movl $1, %1\n\t" "mfence\n\t" "movl %2, %0"
                : "=r"(value), "=m"(state->x)
                : "m"(state->y)
                : "memory");
            state->read0 = value;
        } else {
            uint32_t value;
            __asm__ volatile (
                "movl $1, %1\n\t" "mfence\n\t" "movl %2, %0"
                : "=r"(value), "=m"(state->y)
                : "m"(state->x)
                : "memory");
            state->read1 = value;
        }
        pthread_barrier_wait(&state->done);
    }
    return NULL;
}

static void test_mfence_store_buffering_litmus(void)
{
    struct mfence_litmus state;
    struct mfence_worker_arg args[2] = {{&state, 0}, {&state, 1}};
    pthread_t threads[2];
    memset(&state, 0, sizeof(state));
    int rc = pthread_barrier_init(&state.start, NULL, 3);
    rc |= pthread_barrier_init(&state.done, NULL, 3);
    TEST_ASSERT(rc == 0, "initialize MFENCE litmus barriers");
    if (rc != 0) return;
    rc = pthread_create(&threads[0], NULL, mfence_worker, &args[0]);
    rc |= pthread_create(&threads[1], NULL, mfence_worker, &args[1]);
    TEST_ASSERT(rc == 0, "create MFENCE litmus worker threads");
    if (rc != 0) return;

    unsigned forbidden = 0;
    for (int iteration = 0; iteration < MFENCE_LITMUS_ITERATIONS; iteration++) {
        state.x = 0;
        state.y = 0;
        state.read0 = UINT32_MAX;
        state.read1 = UINT32_MAX;
        pthread_barrier_wait(&state.start);
        pthread_barrier_wait(&state.done);
        if (state.read0 == 0 && state.read1 == 0) forbidden++;
    }
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_barrier_destroy(&state.start);
    pthread_barrier_destroy(&state.done);
    TEST_ASSERT(forbidden == 0,
                "MFENCE forbids store-buffering outcome r0=0,r1=0 (%u occurrences)",
                forbidden);
}

/* Test fence instructions with RDTSC to measure approximate overhead */
static void test_fence_overhead(void)
{
    uint32_t lo1, hi1, lo2, hi2;
    uint64_t tsc_start, tsc_end;

    TEST_START("FENCE - Timing overhead estimate");

    /* Measure LFENCE overhead */
    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo1), "=d"(hi1)
    );
    for (int i = 0; i < 1000; i++) {
        __asm__ volatile ("lfence" ::: "memory");
    }
    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo2), "=d"(hi2)
    );
    tsc_start = ((uint64_t)hi1 << 32) | lo1;
    tsc_end = ((uint64_t)hi2 << 32) | lo2;
    TEST_ASSERT(tsc_end > tsc_start, "LFENCE timing interval is positive");
    printf("  1000x LFENCE: ~%" PRIu64 " cycles (%.1f cycles/fence)\n",
           tsc_end - tsc_start, (double)(tsc_end - tsc_start) / 1000.0);

    /* Measure SFENCE overhead */
    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo1), "=d"(hi1)
    );
    for (int i = 0; i < 1000; i++) {
        __asm__ volatile ("sfence" ::: "memory");
    }
    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo2), "=d"(hi2)
    );
    tsc_start = ((uint64_t)hi1 << 32) | lo1;
    tsc_end = ((uint64_t)hi2 << 32) | lo2;
    TEST_ASSERT(tsc_end > tsc_start, "SFENCE timing interval is positive");
    printf("  1000x SFENCE: ~%" PRIu64 " cycles (%.1f cycles/fence)\n",
           tsc_end - tsc_start, (double)(tsc_end - tsc_start) / 1000.0);

    /* Measure MFENCE overhead */
    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo1), "=d"(hi1)
    );
    for (int i = 0; i < 1000; i++) {
        __asm__ volatile ("mfence" ::: "memory");
    }
    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo2), "=d"(hi2)
    );
    tsc_start = ((uint64_t)hi1 << 32) | lo1;
    tsc_end = ((uint64_t)hi2 << 32) | lo2;
    TEST_ASSERT(tsc_end > tsc_start, "MFENCE timing interval is positive");
    printf("  1000x MFENCE: ~%" PRIu64 " cycles (%.1f cycles/fence)\n",
           tsc_end - tsc_start, (double)(tsc_end - tsc_start) / 1000.0);

}

int main(void)
{
    test_lfence_basic();
    test_sfence_basic();
    test_mfence_basic();
    test_fence_sequence();
    test_lfence_no_flag_change();
    test_sfence_no_flag_change();
    test_mfence_no_flag_change();
    test_sfence_store_ordering();
    test_lfence_load_ordering();
    test_mfence_store_load();
    test_mfence_store_buffering_litmus();
    test_fence_overhead();

    TEST_END();
}
