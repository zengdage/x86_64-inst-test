/*
 * test_xgetbv.c - Test XGETBV instruction
 *
 * XGETBV reads an extended control register (XCR) specified by ECX.
 * Returns the 64-bit value in EDX:EAX.
 * ECX=0 reads XCR0 which indicates which processor state components
 * the OS has enabled for XSAVE/XRSTOR.
 *
 * Prerequisites:
 *   - CPUID.01H:ECX[26] (XSAVE bit) must be set
 *   - CPUID.01H:ECX[27] (OSXSAVE bit) must be set (OS has enabled XSAVE and set CR4.OSXSAVE)
 *
 * Compile: gcc -o test_xgetbv system/test_xgetbv.c -O0
 * Note: Do not use static linking (-static)
 */

#include "../common.h"

static int has_xsave(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    return (ecx >> 26) & 1;
}

static int has_osxsave(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    return (ecx >> 27) & 1;
}

/* Test XGETBV with ECX=0 (read XCR0) */
static void test_xgetbv_xcr0(void)
{
    uint32_t lo, hi;
    uint64_t xcr0;

    TEST_START("XGETBV - Read XCR0 (ECX=0)");

    __asm__ volatile (
        "xgetbv"
        : "=a"(lo), "=d"(hi)
        : "c"(0)
    );

    xcr0 = ((uint64_t)hi << 32) | lo;
    printf("  XCR0 = 0x%016" PRIX64 "\n", xcr0);

    /* Bit 0 (x87 FPU) must always be set */
    TEST_ASSERT(xcr0 & (1ULL << 0), "XCR0 bit 0 (x87 FPU/MMX) must be set");

    /* Report state component bits */
    printf("  x87 FPU/MMX (bit 0):    %s\n", (xcr0 & (1ULL << 0)) ? "enabled" : "disabled");
    printf("  SSE (bit 1):            %s\n", (xcr0 & (1ULL << 1)) ? "enabled" : "disabled");
    printf("  AVX (bit 2):            %s\n", (xcr0 & (1ULL << 2)) ? "enabled" : "disabled");
    printf("  BNDREG (bit 3):         %s\n", (xcr0 & (1ULL << 3)) ? "enabled" : "disabled");
    printf("  BNDCSR (bit 4):         %s\n", (xcr0 & (1ULL << 4)) ? "enabled" : "disabled");
    printf("  Opmask/k-regs (bit 5):  %s\n", (xcr0 & (1ULL << 5)) ? "enabled" : "disabled");
    printf("  ZMM_Hi256 (bit 6):      %s\n", (xcr0 & (1ULL << 6)) ? "enabled" : "disabled");
    printf("  Hi16_ZMM (bit 7):       %s\n", (xcr0 & (1ULL << 7)) ? "enabled" : "disabled");
    printf("  PKRU (bit 9):           %s\n", (xcr0 & (1ULL << 9)) ? "enabled" : "disabled");

    /* On x86-64 with SSE2 (mandatory), bit 1 should also be set */
    TEST_ASSERT(xcr0 & (1ULL << 1), "XCR0 bit 1 (SSE) should be set on x86-64");
}

/* Test XGETBV consistency */
static void test_xgetbv_consistency(void)
{
    uint32_t lo1, hi1, lo2, hi2;
    uint64_t xcr0_1, xcr0_2;

    TEST_START("XGETBV - Consistency (two reads)");

    __asm__ volatile (
        "xgetbv"
        : "=a"(lo1), "=d"(hi1)
        : "c"(0)
    );

    __asm__ volatile (
        "xgetbv"
        : "=a"(lo2), "=d"(hi2)
        : "c"(0)
    );

    xcr0_1 = ((uint64_t)hi1 << 32) | lo1;
    xcr0_2 = ((uint64_t)hi2 << 32) | lo2;

    TEST_ASSERT(xcr0_1 == xcr0_2,
                "Two XGETBV reads should return same value: 0x%" PRIX64 " vs 0x%" PRIX64,
                xcr0_1, xcr0_2);
}

/* Test XCR0 and CPUID feature consistency */
static void test_xgetbv_cpuid_consistency(void)
{
    uint32_t lo, hi;
    uint64_t xcr0;
    uint32_t eax, ebx, ecx, edx;

    TEST_START("XGETBV - XCR0 vs CPUID feature consistency");

    __asm__ volatile (
        "xgetbv"
        : "=a"(lo), "=d"(hi)
        : "c"(0)
    );
    xcr0 = ((uint64_t)hi << 32) | lo;

    /* Check CPUID leaf 1 for AVX */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    int cpuid_avx = (ecx >> 28) & 1;
    int xcr0_avx = (xcr0 >> 2) & 1;

    printf("  CPUID AVX: %d, XCR0 AVX (bit 2): %d\n", cpuid_avx, xcr0_avx);

    /* If CPU supports AVX and OS has enabled it, XCR0 bit 2 should be set.
     * But if CPUID says no AVX, XCR0 bit 2 should be clear. */
    if (!cpuid_avx) {
        TEST_ASSERT(!xcr0_avx,
                     "XCR0 AVX bit should be 0 when CPU doesn't support AVX");
    } else {
        /* AVX in CPUID means hardware support exists; OS may or may not enable it */
        printf("  CPU supports AVX; OS %s enabled it in XCR0\n",
               xcr0_avx ? "has" : "has NOT");
        TEST_ASSERT(1, "AVX hardware support detected; XCR0 AVX bit = %d", xcr0_avx);
    }
}

/* Test XGETBV does not modify flags */
static void test_xgetbv_no_flag_change(void)
{
    uint64_t flags_before, flags_after;

    TEST_START("XGETBV - Does not modify flags");

    __asm__ volatile (
        "pushfq\n\t"
        "popq %0\n\t"
        "xgetbv\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        : "c"(0)
        : "eax", "edx"
    );

    TEST_ASSERT(flags_before == flags_after,
                "XGETBV should not change flags: before=0x%" PRIX64 ", after=0x%" PRIX64,
                flags_before, flags_after);
}

int main(void)
{
    printf("Checking XSAVE/OSXSAVE support...\n");

    if (!has_xsave()) {
        printf("XSAVE not supported by CPU (CPUID.01H:ECX[26] = 0). Skipping.\n");
        TEST_ASSERT(1, "XSAVE not available - skip is expected");
        TEST_END();
    }

    if (!has_osxsave()) {
        printf("OSXSAVE not set (CPUID.01H:ECX[27] = 0). OS has not enabled XSAVE. Skipping.\n");
        TEST_ASSERT(1, "OSXSAVE not set - skip is expected");
        TEST_END();
    }

    printf("  XSAVE and OSXSAVE are both supported and enabled\n\n");

    test_xgetbv_xcr0();
    test_xgetbv_consistency();
    test_xgetbv_cpuid_consistency();
    test_xgetbv_no_flag_change();

    TEST_END();
}
