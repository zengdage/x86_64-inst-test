/*
 * test_clflush.c - Test x86-64 CLFLUSH/CLFLUSHOPT instructions
 *
 * CLFLUSH: Flush a cache line containing the specified address.
 * CLFLUSHOPT: Optimized cache line flush (weakly ordered).
 * Neither affects registers or flags (except side effects on cache).
 * These are hard to directly verify; we just ensure they execute without faulting.
 *
 * Compile: gcc -o test_clflush integer/test_clflush.c -O0 -mclflushopt
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_clflush_basic(void) {
    volatile uint64_t data __attribute__((aligned(64))) = 42;

    /* CLFLUSH should not fault on valid memory */
    __asm__ volatile (
        "clflush %0"
        :
        : "m"(data)
        : "memory"
    );
    TEST_ASSERT(data == 42, "clflush: data still accessible after flush");

    /* Read after flush should still work (data reloaded from memory) */
    data = 99;
    __asm__ volatile (
        "clflush %0"
        :
        : "m"(data)
        : "memory"
    );
    TEST_ASSERT(data == 99, "clflush: data correct after write+flush");
}

static void test_clflush_array(void) {
    volatile uint64_t arr[64] __attribute__((aligned(64)));
    for (int i = 0; i < 64; i++) arr[i] = i;

    /* Flush multiple cache lines */
    for (int i = 0; i < 64; i += 8) {  /* assuming 64-byte cache lines */
        __asm__ volatile (
            "clflush %0"
            :
            : "m"(arr[i])
            : "memory"
        );
    }

    /* Verify data still correct */
    int correct = 1;
    for (int i = 0; i < 64; i++) {
        if (arr[i] != (uint64_t)i) { correct = 0; break; }
    }
    TEST_ASSERT(correct, "clflush array: data intact after flushing multiple lines");
}

static void test_clflushopt_basic(void) {
    volatile uint64_t data __attribute__((aligned(64))) = 123;

    __asm__ volatile (
        "clflushopt %0"
        :
        : "m"(data)
        : "memory"
    );
    TEST_ASSERT(data == 123, "clflushopt: data still accessible after flush");
}

static void test_clflush_no_flags(void) {
    volatile uint64_t data __attribute__((aligned(64))) = 0;
    uint64_t flags_before, flags_after;

    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "clflush %2\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        : "m"(data)
        : "memory", "cc"
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "clflush: flags unchanged");
}

int main(void) {
    TEST_START("CLFLUSH/CLFLUSHOPT instructions");
    test_clflush_basic();
    test_clflush_array();
    test_clflushopt_basic();
    test_clflush_no_flags();
    TEST_END();
}
