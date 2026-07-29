/*
 * test_prefetch.c - Test x86-64 PREFETCH instructions
 *
 * PREFETCHT0:  Prefetch data to all cache levels.
 * PREFETCHT1:  Prefetch data to L2 and above.
 * PREFETCHT2:  Prefetch data to L3 and above.
 * PREFETCHNTA: Prefetch data with non-temporal hint.
 * PREFETCHW:   Prefetch data for writing (3DNow! / PREFETCHW extension).
 * These are hints; they do not affect correctness or flags.
 * We just verify they execute without faulting.
 *
 * Compile: gcc -o test_prefetch integer/test_prefetch.c -O0 -mprfchw
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_prefetcht0(void) {
    volatile uint64_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    __asm__ volatile ("prefetcht0 %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 1, "prefetcht0: data accessible");
}

static void test_prefetcht1(void) {
    volatile uint64_t data[8] = {10, 20, 30, 40, 50, 60, 70, 80};

    __asm__ volatile ("prefetcht1 %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 10, "prefetcht1: data accessible");
}

static void test_prefetcht2(void) {
    volatile uint64_t data[8] = {100, 200, 300, 400, 500, 600, 700, 800};

    __asm__ volatile ("prefetcht2 %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 100, "prefetcht2: data accessible");
}

static void test_prefetchnta(void) {
    volatile uint64_t data[8] = {0};

    __asm__ volatile ("prefetchnta %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 0, "prefetchnta: data accessible");
}

static void test_prefetch_no_flags(void) {
    volatile uint64_t data = 42;
    uint64_t flags_before, flags_after;

    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "prefetcht0 %2\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        : "m"(data)
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "prefetch: flags unchanged");
}

static void test_prefetch_array(void) {
    volatile uint64_t arr[128];
    for (int i = 0; i < 128; i++) arr[i] = i;

    /* Prefetch multiple cache lines */
    for (int i = 0; i < 128; i += 8) {
        __asm__ volatile ("prefetcht0 %0" : : "m"(arr[i]));
    }

    /* Verify data still correct */
    int correct = 1;
    for (int i = 0; i < 128; i++) {
        if (arr[i] != (uint64_t)i) { correct = 0; break; }
    }
    TEST_ASSERT(correct, "prefetch array: data intact");
}

int main(void) {
    TEST_START("PREFETCH instructions");
    test_prefetcht0();
    test_prefetcht1();
    test_prefetcht2();
    test_prefetchnta();
    test_prefetch_no_flags();
    test_prefetch_array();
    TEST_END();
}
