/*
 * Test CVTPD2PS packed double-to-float conversion.
 * Compile: gcc -o test_cvtpd2ps float/test_cvtpd2ps.c -O0
 * Do NOT use static linking.
 */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include "../common.h"

static void cvtpd2ps_register(const xmm_t* input, float output[2]) {
    __asm__ volatile(
        "movapd %1, %%xmm1\n\t"
        "cvtpd2ps %%xmm1, %%xmm0\n\t"
        "movlps %%xmm0, %0"
        : "=m"(output[0]) : "m"(*input) : "xmm0", "xmm1");
}

static void cvtpd2ps_memory(const xmm_t* input, float output[2]) {
    __asm__ volatile(
        "cvtpd2ps %1, %%xmm0\n\t"
        "movlps %%xmm0, %0"
        : "=m"(output[0]) : "m"(*input) : "xmm0");
}

static void test_cvtpd2ps_basic(void) {
    TEST_START("CVTPD2PS (128-bit: 2 double->float)");
    const xmm_t input = { .f64 = {1.5, -2.5} };
    float output[2];

    cvtpd2ps_register(&input, output);
    TEST_ASSERT(output[0] == 1.5f, "cvtpd2ps register lane 0");
    TEST_ASSERT(output[1] == -2.5f, "cvtpd2ps register lane 1");

    cvtpd2ps_memory(&input, output);
    TEST_ASSERT(output[0] == 1.5f, "cvtpd2ps memory lane 0");
    TEST_ASSERT(output[1] == -2.5f, "cvtpd2ps memory lane 1");
}

static void test_cvtpd2ps_specials(void) {
    xmm_t input;
    float output[2];
    uint32_t saved, csr, after;

    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    csr = (saved & ~((UINT32_C(3) << 13) | UINT32_C(0x3f) |
                     (UINT32_C(1) << 6) | (UINT32_C(1) << 15))) |
          (UINT32_C(0x3f) << 7);

    input.u64[0] = UINT64_C(0x7ff8000000001234);
    input.f64[1] = INFINITY;
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    cvtpd2ps_register(&input, output);
    TEST_ASSERT(IS_QNAN(output[0]) && !IS_SNAN(output[0]),
                "cvtpd2ps QNaN result is quiet");
    TEST_ASSERT(isinf(output[1]) && !signbit(output[1]), "cvtpd2ps +Inf");

    input.u64[0] = UINT64_C(0x7ff0000000001234);
    input.f64[1] = -INFINITY;
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    cvtpd2ps_register(&input, output);
    __asm__ volatile("stmxcsr %0" : "=m"(after));
    TEST_ASSERT(IS_QNAN(output[0]) && !IS_SNAN(output[0]),
                "cvtpd2ps quiets SNaN");
    TEST_ASSERT(isinf(output[1]) && signbit(output[1]), "cvtpd2ps -Inf");
    TEST_ASSERT(after & UINT32_C(1), "cvtpd2ps SNaN sets invalid flag");

    input.f64[0] = DBL_MAX;
    input.f64[1] = -DBL_MAX;
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    cvtpd2ps_register(&input, output);
    TEST_ASSERT(isinf(output[0]) && !signbit(output[0]), "cvtpd2ps +overflow");
    TEST_ASSERT(isinf(output[1]) && signbit(output[1]), "cvtpd2ps -overflow");

    input.f64[0] = 0.0;
    input.f64[1] = -0.0;
    cvtpd2ps_register(&input, output);
    TEST_ASSERT(output[0] == 0.0f && !signbit(output[0]), "cvtpd2ps +0 sign");
    TEST_ASSERT(output[1] == 0.0f && signbit(output[1]), "cvtpd2ps -0 sign");

    input.f64[0] = 0x1p-149;
    input.f64[1] = -0x1p-149;
    cvtpd2ps_register(&input, output);
    TEST_ASSERT(output[0] == 0x1p-149f, "cvtpd2ps +minimum subnormal");
    TEST_ASSERT(output[1] == -0x1p-149f, "cvtpd2ps -minimum subnormal");

    input.f64[0] = DBL_MIN;
    input.f64[1] = -DBL_MIN;
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    cvtpd2ps_register(&input, output);
    __asm__ volatile("stmxcsr %0" : "=m"(after));
    TEST_ASSERT(output[0] == 0.0f && !signbit(output[0]), "cvtpd2ps +underflow");
    TEST_ASSERT(output[1] == 0.0f && signbit(output[1]), "cvtpd2ps -underflow");
    TEST_ASSERT((after & UINT32_C(0x30)) == UINT32_C(0x30),
                "cvtpd2ps underflow sets underflow and precision flags");

    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
}

static void test_cvtpd2ps_mxcsr_rounding(void) {
    /* Values are 0.75 float ULP away from +/-1.0. */
    const xmm_t input = { .f64 = {0x1.0000018p+0, -0x1.0000018p+0} };
    const float expected[4][2] = {
        {0x1.000002p+0f, -0x1.000002p+0f},
        {0x1p+0f, -0x1.000002p+0f},
        {0x1.000002p+0f, -0x1p+0f},
        {0x1p+0f, -0x1p+0f}
    };
    float output[2];
    uint32_t saved, csr;

    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    for (uint32_t mode = 0; mode < 4; mode++) {
        csr = (saved & ~((UINT32_C(3) << 13) | UINT32_C(0x3f))) | (mode << 13);
        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        cvtpd2ps_register(&input, output);
        for (int lane = 0; lane < 2; lane++)
            TEST_ASSERT(output[lane] == expected[mode][lane],
                        "cvtpd2ps register MXCSR mode %u lane %d", mode, lane);

        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        cvtpd2ps_memory(&input, output);
        for (int lane = 0; lane < 2; lane++)
            TEST_ASSERT(output[lane] == expected[mode][lane],
                        "cvtpd2ps memory MXCSR mode %u lane %d", mode, lane);
    }
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
}

int main(void) {
    test_cvtpd2ps_basic();
    test_cvtpd2ps_specials();
    test_cvtpd2ps_mxcsr_rounding();
    TEST_END();
}
