/* TARGET: VCVTSS2USI */
#include "../include/avx512_test.h"

static uint64_t convert(float value) {
    uint64_t result;
    __asm__ volatile("vmovss %1, %%xmm0\n\t"
                     "vcvtss2usi %%xmm0, %0"
                     : "=r"(result) : "m"(value) : "xmm0");
    return result;
}

int main(void) {
    TEST_START("VCVTSS2USI");
    if (!CHECK_AVX512F()) return 0;

    uint32_t saved_mxcsr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved_mxcsr));
    const uint32_t clean = saved_mxcsr & ~UINT32_C(0x6000);
    uint32_t mxcsr = clean;
    __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));

    TEST_ASSERT(convert(0.0f) == 0, "VCVTSS2USI zero");
    TEST_ASSERT(convert(1.0f) == 1, "VCVTSS2USI one");
    TEST_ASSERT(convert(2.5f) == 2, "VCVTSS2USI nearest ties-to-even");
    TEST_ASSERT(convert(3.5f) == 4, "VCVTSS2USI nearest upper tie");
    TEST_ASSERT(convert(4294967296.0f) == UINT64_C(4294967296),
                "VCVTSS2USI 64-bit destination");

    mxcsr = clean | UINT32_C(0x4000);
    __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));
    TEST_ASSERT(convert(2.25f) == 3, "VCVTSS2USI round upward");

    mxcsr = clean | UINT32_C(0x6000);
    __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));
    TEST_ASSERT(convert(2.75f) == 2, "VCVTSS2USI round toward zero");

    __asm__ volatile("ldmxcsr %0" : : "m"(saved_mxcsr));
    TEST_END();
}
