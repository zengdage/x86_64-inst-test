/*
 * test_rdtsc.c - Test x86-64 RDTSC/RDTSCP instructions
 *
 * RDTSC: Read Time-Stamp Counter. Returns 64-bit TSC in EDX:EAX.
 * RDTSCP: Read TSC and processor ID. Also returns IA32_TSC_AUX in ECX.
 * Neither affects flags.
 *
 * Compile: gcc -o test_rdtsc integer/test_rdtsc.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_rdtsc_basic(void) {
    uint32_t lo, hi;
    uint64_t tsc;

    __asm__ volatile (
        "rdtsc"
        : "=a"(lo), "=d"(hi)
        :
        : "cc"
    );
    tsc = ((uint64_t)hi << 32) | lo;
    TEST_ASSERT(tsc > 0, "rdtsc: TSC should be > 0, got %lu", tsc);
}

static void test_rdtsc_monotonic(void) {
    uint32_t lo1, hi1, lo2, hi2;
    uint64_t tsc1, tsc2;

    __asm__ volatile ("rdtsc" : "=a"(lo1), "=d"(hi1) : : "cc");
    tsc1 = ((uint64_t)hi1 << 32) | lo1;

    /* Do some work between reads */
    volatile int x = 0;
    for (int i = 0; i < 1000; i++) x += i;
    (void)x;

    __asm__ volatile ("rdtsc" : "=a"(lo2), "=d"(hi2) : : "cc");
    tsc2 = ((uint64_t)hi2 << 32) | lo2;

    TEST_ASSERT(tsc2 > tsc1, "rdtsc: TSC should be monotonic (tsc1=%lu, tsc2=%lu)", tsc1, tsc2);
}

static void test_rdtscp(void) {
    uint32_t lo, hi, aux;
    uint64_t tsc;

    __asm__ volatile (
        "rdtscp"
        : "=a"(lo), "=d"(hi), "=c"(aux)
        :
        : "cc"
    );
    tsc = ((uint64_t)hi << 32) | lo;
    TEST_ASSERT(tsc > 0, "rdtscp: TSC should be > 0");
    printf("  RDTSCP: TSC=%lu, AUX(processor ID)=%u\n", tsc, aux);
}

static void test_rdtscp_after_rdtsc(void) {
    uint32_t lo1, hi1, lo2, hi2, aux;
    uint64_t tsc1, tsc2;

    __asm__ volatile ("rdtsc" : "=a"(lo1), "=d"(hi1));
    tsc1 = ((uint64_t)hi1 << 32) | lo1;

    __asm__ volatile ("rdtscp" : "=a"(lo2), "=d"(hi2), "=c"(aux));
    tsc2 = ((uint64_t)hi2 << 32) | lo2;

    TEST_ASSERT(tsc2 >= tsc1, "rdtscp >= rdtsc: ordering");
}

int main(void) {
    TEST_START("RDTSC/RDTSCP instructions");
    test_rdtsc_basic();
    test_rdtsc_monotonic();
    test_rdtscp();
    test_rdtscp_after_rdtsc();
    TEST_END();
}
