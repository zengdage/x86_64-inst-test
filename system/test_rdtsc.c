/*
 * test_rdtsc.c - Test RDTSC and RDTSCP instructions
 *
 * RDTSC reads the 64-bit Time Stamp Counter into EDX:EAX.
 * RDTSCP also reads TSC and additionally returns the IA32_TSC_AUX value in ECX.
 * RDTSCP is a serializing read (waits for prior instructions to complete).
 *
 * Prerequisites: TSC support (universal on x86-64), RDTSCP requires CPUID.80000001H:EDX[27]
 *
 * Compile: gcc -o test_rdtsc system/test_rdtsc.c -O0
 * Note: Do not use static linking (-static)
 */

#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int has_rdtscp(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000001)
    );
    return (edx >> 27) & 1;
}
#else
#define has_rdtscp() 1
#endif

/* Test basic RDTSC */
static void test_rdtsc_basic(void)
{
    uint32_t lo, hi;
    uint64_t tsc;

    TEST_START("RDTSC - Basic Read");

    __asm__ volatile (
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );

    tsc = ((uint64_t)hi << 32) | lo;
    printf("  TSC value: %" PRIu64 " (0x%016" PRIX64 ")\n", tsc, tsc);
    TEST_ASSERT(tsc > 0, "TSC value should be non-zero, got %" PRIu64, tsc);
}

/* Test RDTSC monotonicity */
static void test_rdtsc_monotonic(void)
{
    uint32_t lo1, hi1, lo2, hi2;
    uint64_t tsc1, tsc2;

    TEST_START("RDTSC - Monotonicity");

    __asm__ volatile (
        "rdtsc"
        : "=a"(lo1), "=d"(hi1)
    );

    /* Small busy loop to let time pass */
    for (volatile int i = 0; i < 1000; i++);

    __asm__ volatile (
        "rdtsc"
        : "=a"(lo2), "=d"(hi2)
    );

    tsc1 = ((uint64_t)hi1 << 32) | lo1;
    tsc2 = ((uint64_t)hi2 << 32) | lo2;

    printf("  TSC1: %" PRIu64 "\n", tsc1);
    printf("  TSC2: %" PRIu64 "\n", tsc2);
    printf("  Delta: %" PRIu64 "\n", tsc2 - tsc1);

    TEST_ASSERT(tsc2 > tsc1, "Second RDTSC should be greater than first");
}

/* Test multiple RDTSC reads for consistent increasing values */
static void test_rdtsc_multiple(void)
{
    uint64_t tscs[10];
    uint32_t lo, hi;
    int all_increasing = 1;

    TEST_START("RDTSC - Multiple Reads Increasing");

    for (int i = 0; i < 10; i++) {
        __asm__ volatile (
            "rdtsc"
            : "=a"(lo), "=d"(hi)
        );
        tscs[i] = ((uint64_t)hi << 32) | lo;
    }

    for (int i = 1; i < 10; i++) {
        if (tscs[i] <= tscs[i - 1]) {
            all_increasing = 0;
            printf("  TSC[%d]=%" PRIu64 " <= TSC[%d]=%" PRIu64 "\n",
                   i, tscs[i], i - 1, tscs[i - 1]);
        }
    }

    printf("  TSC range: %" PRIu64 " to %" PRIu64 " (delta=%" PRIu64 ")\n",
           tscs[0], tscs[9], tscs[9] - tscs[0]);
    TEST_ASSERT(all_increasing, "All sequential RDTSC reads should be strictly increasing");
}

/* Test RDTSCP if supported */
static void test_rdtscp_basic(void)
{
    uint32_t lo, hi, aux;
    uint64_t tsc;

    TEST_START("RDTSCP - Basic Read with Processor ID");

    if (!has_rdtscp()) {
        printf("  RDTSCP not supported, skipping\n");
        return;
    }

    __asm__ volatile (
        "rdtscp"
        : "=a"(lo), "=d"(hi), "=c"(aux)
    );

    tsc = ((uint64_t)hi << 32) | lo;
    printf("  TSC value: %" PRIu64 "\n", tsc);
    printf("  IA32_TSC_AUX (processor ID): %u (0x%08X)\n", aux, aux);

    TEST_ASSERT(tsc > 0, "RDTSCP TSC value should be non-zero");
}

/* Test RDTSCP monotonicity */
static void test_rdtscp_monotonic(void)
{
    uint32_t lo1, hi1, aux1, lo2, hi2, aux2;
    uint64_t tsc1, tsc2;

    TEST_START("RDTSCP - Monotonicity");

    if (!has_rdtscp()) {
        printf("  RDTSCP not supported, skipping\n");
        return;
    }

    __asm__ volatile (
        "rdtscp"
        : "=a"(lo1), "=d"(hi1), "=c"(aux1)
    );

    for (volatile int i = 0; i < 1000; i++);

    __asm__ volatile (
        "rdtscp"
        : "=a"(lo2), "=d"(hi2), "=c"(aux2)
    );

    tsc1 = ((uint64_t)hi1 << 32) | lo1;
    tsc2 = ((uint64_t)hi2 << 32) | lo2;

    printf("  TSC1: %" PRIu64 " (aux=%u)\n", tsc1, aux1);
    printf("  TSC2: %" PRIu64 " (aux=%u)\n", tsc2, aux2);
    printf("  Delta: %" PRIu64 "\n", tsc2 - tsc1);

    TEST_ASSERT(tsc2 > tsc1, "Second RDTSCP should be greater than first");
}

/* Test LFENCE+RDTSC as serializing alternative */
static void test_lfence_rdtsc(void)
{
    uint32_t lo1, hi1, lo2, hi2;
    uint64_t tsc1, tsc2;

    TEST_START("LFENCE+RDTSC - Serialized Timestamp Read");

    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo1), "=d"(hi1)
    );

    for (volatile int i = 0; i < 100; i++);

    __asm__ volatile (
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo2), "=d"(hi2)
    );

    tsc1 = ((uint64_t)hi1 << 32) | lo1;
    tsc2 = ((uint64_t)hi2 << 32) | lo2;

    printf("  Serialized TSC1: %" PRIu64 "\n", tsc1);
    printf("  Serialized TSC2: %" PRIu64 "\n", tsc2);
    TEST_ASSERT(tsc2 > tsc1, "Serialized RDTSC reads should be increasing");
}

static void test_timestamp_flags(void)
{
    uint64_t before, after_rdtsc, after_rdtscp;
    uint32_t lo, hi, aux;
    __asm__ volatile (
        "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "pushfq\n\t" "popq %0\n\t" "rdtsc\n\t"
        "pushfq\n\t" "popq %1"
        : "=&r"(before), "=&r"(after_rdtsc), "=a"(lo), "=d"(hi)
        :
        : "r11", "cc");
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((before & mask) == (after_rdtsc & mask), "RDTSC preserves status flags");
    TEST_ASSERT((((uint64_t)hi << 32) | lo) != 0, "RDTSC flags test returns a timestamp");

    if (has_rdtscp()) {
        __asm__ volatile (
            "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
            "rdtscp\n\t" "pushfq\n\t" "popq %0"
            : "=&r"(after_rdtscp), "=a"(lo), "=d"(hi), "=c"(aux)
            :
            : "r11", "cc");
        TEST_ASSERT((before & mask) == (after_rdtscp & mask), "RDTSCP preserves status flags");
        TEST_ASSERT((((uint64_t)hi << 32) | lo) != 0, "RDTSCP flags test returns a timestamp");
        (void)aux;
    }
}

int main(void)
{
    test_rdtsc_basic();
    test_rdtsc_monotonic();
    test_rdtsc_multiple();
    test_rdtscp_basic();
    test_rdtscp_monotonic();
    test_lfence_rdtsc();
    test_timestamp_flags();

    TEST_END();
}
