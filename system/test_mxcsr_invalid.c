/*
 * test_mxcsr_invalid.c - Test STMXCSR/VSTMXCSR after a masked invalid operation
 *
 * STMXCSR and VSTMXCSR must both report MXCSR exception status bits.  The
 * invalid-operation exception from DIVSS 0/0 is masked so the test can check
 * the saved state without raising SIGFPE.
 */
#include "../common.h"

#if ENABLE_MXCSR_CHECK

#define MXCSR_DEFAULT UINT32_C(0x1f80)
#define MXCSR_INVALID_FLAG UINT32_C(0x0001)

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1), "c"(0));
    if ((ecx & UINT32_C(0x18000000)) != UINT32_C(0x18000000))
        return 0;

    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return (eax & 6u) == 6u;
}
#else
#define check_avx() 1
#endif

static void test_stmxcsr_vstmxcsr_invalid_flag(void)
{
    const uint32_t test_mxcsr = MXCSR_DEFAULT;
    const uint32_t expected_mxcsr = test_mxcsr | MXCSR_INVALID_FLAG;
    uint32_t saved_mxcsr;
    uint32_t stmxcsr;
    uint32_t vstmxcsr;

    TEST_START("STMXCSR/VSTMXCSR - Invalid Operation");

    __asm__ volatile(
        "stmxcsr %[saved_mxcsr]\n\t"
        "ldmxcsr %[test_mxcsr]\n\t"
        "xorps %%xmm0, %%xmm0\n\t"
        "divss %%xmm0, %%xmm0\n\t"
        "stmxcsr %[stmxcsr]\n\t"
        "vstmxcsr %[vstmxcsr]\n\t"
        "ldmxcsr %[saved_mxcsr]"
        : [saved_mxcsr] "=m"(saved_mxcsr),
          [stmxcsr] "=m"(stmxcsr),
          [vstmxcsr] "=m"(vstmxcsr)
        : [test_mxcsr] "m"(test_mxcsr)
        : "xmm0", "memory");

    TEST_ASSERT(stmxcsr == expected_mxcsr,
                "STMXCSR after DIVSS 0/0: got %08" PRIx32
                ", expected %08" PRIx32, stmxcsr, expected_mxcsr);
    TEST_ASSERT(vstmxcsr == expected_mxcsr,
                "VSTMXCSR after DIVSS 0/0: got %08" PRIx32
                ", expected %08" PRIx32, vstmxcsr, expected_mxcsr);
}
#endif

int main(void)
{
#if ENABLE_MXCSR_CHECK
    if (!check_avx()) {
        puts("  SKIP: AVX or OS support for XMM/YMM state is unavailable");
        TEST_END();
    }
    test_stmxcsr_vstmxcsr_invalid_flag();
#else
    puts("  SKIP: rebuild with ENABLE_MXCSR_CHECK=1 to check MXCSR exceptions");
#endif
    TEST_END();
}
