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

#include "../common.h"

/* Test LFENCE executes without fault */
static void test_lfence_basic(void)
{
    TEST_START("LFENCE - Basic execution");

    __asm__ volatile ("lfence");

    TEST_ASSERT(1, "LFENCE executed successfully");
}

/* Test SFENCE executes without fault */
static void test_sfence_basic(void)
{
    TEST_START("SFENCE - Basic execution");

    __asm__ volatile ("sfence");

    TEST_ASSERT(1, "SFENCE executed successfully");
}

/* Test MFENCE executes without fault */
static void test_mfence_basic(void)
{
    TEST_START("MFENCE - Basic execution");

    __asm__ volatile ("mfence");

    TEST_ASSERT(1, "MFENCE executed successfully");
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

    TEST_ASSERT(1, "Sequence of SFENCE/LFENCE/MFENCE executed successfully");
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
    printf("  1000x MFENCE: ~%" PRIu64 " cycles (%.1f cycles/fence)\n",
           tsc_end - tsc_start, (double)(tsc_end - tsc_start) / 1000.0);

    /* MFENCE should generally be more expensive than LFENCE/SFENCE */
    TEST_ASSERT(1, "Fence overhead measurement completed");
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
    test_fence_overhead();

    TEST_END();
}
