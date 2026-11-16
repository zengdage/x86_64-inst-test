#ifndef X86_64_AVX_CVT_TEST_H
#define X86_64_AVX_CVT_TEST_H

#include "../common.h"
#include <limits.h>
#include <math.h>

#define CVT_DQ2PD 1
#define CVT_DQ2PS 2
#define CVT_PD2DQ 3
#define CVT_PD2PS 4
#define CVT_PS2DQ 5
#define CVT_PS2PD 6
#define CVT_SD2SI 7
#define CVT_SD2SS 8
#define CVT_SI2SD 9
#define CVT_SI2SS 10
#define CVT_SS2SD 11
#define CVT_SS2SI 12
#define CVT_TPD2DQ 13
#define CVT_TPS2DQ 14
#define CVT_TSD2SI 15
#define CVT_TSS2SI 16

#if TEST_CVT == CVT_PD2DQ || TEST_CVT == CVT_PS2DQ || \
    TEST_CVT == CVT_SD2SI || TEST_CVT == CVT_SS2SI || \
    TEST_CVT == CVT_TPD2DQ || TEST_CVT == CVT_TPS2DQ || \
    TEST_CVT == CVT_TSD2SI || TEST_CVT == CVT_TSS2SI
static uint32_t use_round_to_nearest(void) {
    uint32_t original, nearest;
    __asm__ volatile("stmxcsr %0" : "=m"(original));
    nearest = original & ~UINT32_C(0x6000);
    __asm__ volatile("ldmxcsr %0" : : "m"(nearest));
    return original;
}

static void restore_mxcsr(uint32_t value) {
    __asm__ volatile("ldmxcsr %0" : : "m"(value));
}
#endif

#if TEST_CVT == CVT_DQ2PD
int main(void) {
    xmm_t src = { .i32 = {INT32_MIN, -1, 16777217, INT32_MAX} };
    ymm_t out;
    const double expected[4] = {-2147483648.0, -1.0, 16777217.0, 2147483647.0};
    TEST_START("VCVTDQ2PD");
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvcvtdq2pd %%xmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "xmm0", "ymm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.f64[i] == expected[i], "VCVTDQ2PD lane %d", i);
    __asm__ volatile("vcvtdq2pd %1, %%xmm1\n\tvmovdqu %%xmm1, %0"
                     : "=m"(out) : "m"(src) : "xmm1");
    TEST_ASSERT(out.f64[0] == expected[0] && out.f64[1] == expected[1],
                "VCVTDQ2PD VEX.128 memory form");
    TEST_END();
}
#elif TEST_CVT == CVT_DQ2PS
int main(void) {
    ymm_t src = { .i32 = {INT32_MIN, -16777217, -1, 0, 1, 16777217, INT32_MAX, 42} };
    ymm_t out;
    TEST_START("VCVTDQ2PS");
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvcvtdq2ps %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "ymm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f32[i] == (float)src.i32[i], "VCVTDQ2PS lane %d", i);
    TEST_END();
}
#elif TEST_CVT == CVT_PD2DQ || TEST_CVT == CVT_TPD2DQ
int main(void) {
    ymm_t src = { .f64 = {1.5, 2.5, -1.5, 2147483648.0} };
    ymm_t out;
    uint32_t original = use_round_to_nearest();
    const int32_t expected[4] = {
#if TEST_CVT == CVT_TPD2DQ
        1, 2, -1, INT32_MIN
#else
        2, 2, -2, INT32_MIN
#endif
    };
    TEST_START(TEST_NAME);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" TEST_INSN " %%ymm0, %%xmm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "ymm0", "xmm1");
    restore_mxcsr(original);
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.i32[i] == expected[i], TEST_NAME " lane %d", i);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(out.u32[i] == 0, TEST_NAME " zeroes upper YMM lane %d", i);
    TEST_END();
}
#elif TEST_CVT == CVT_PD2PS
int main(void) {
    ymm_t src = { .f64 = {1.0, -0.0, 16777217.0, 0x1p-149} };
    ymm_t out;
    TEST_START("VCVTPD2PS");
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvcvtpd2ps %%ymm0, %%xmm1\n\t"
                     "vmovdqu %%xmm1, %0"
                     : "=m"(out) : "m"(src) : "ymm0", "xmm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.f32[i] == (float)src.f64[i], "VCVTPD2PS lane %d", i);
    TEST_ASSERT(out.u32[1] == UINT32_C(0x80000000), "VCVTPD2PS preserves -0");
    for (int i = 4; i < 8; i++) TEST_ASSERT(out.u32[i] == 0, "VCVTPD2PS upper lane %d", i);
    TEST_END();
}
#elif TEST_CVT == CVT_PS2DQ || TEST_CVT == CVT_TPS2DQ
int main(void) {
    ymm_t src = { .f32 = {1.5f, 2.5f, -1.5f, 2147483648.0f,
                          -2147483904.0f, 0.0f, NAN, INFINITY} };
    ymm_t out;
    uint32_t original = use_round_to_nearest();
    const int32_t expected[8] = {
#if TEST_CVT == CVT_TPS2DQ
        1, 2, -1, INT32_MIN, INT32_MIN, 0, INT32_MIN, INT32_MIN
#else
        2, 2, -2, INT32_MIN, INT32_MIN, 0, INT32_MIN, INT32_MIN
#endif
    };
    TEST_START(TEST_NAME);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" TEST_INSN " %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "ymm0", "ymm1");
    restore_mxcsr(original);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.i32[i] == expected[i], TEST_NAME " lane %d got %d", i, out.i32[i]);
    TEST_END();
}
#elif TEST_CVT == CVT_PS2PD
int main(void) {
    xmm_t src = { .u32 = {UINT32_C(0x00000001), UINT32_C(0x80000000),
                           UINT32_C(0x3f800000), UINT32_C(0xff800000)} };
    ymm_t out;
    TEST_START("VCVTPS2PD");
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvcvtps2pd %%xmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "xmm0", "ymm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.f64[i] == (double)src.f32[i], "VCVTPS2PD lane %d", i);
    TEST_ASSERT(signbit(out.f64[1]), "VCVTPS2PD preserves -0");
    TEST_END();
}
#elif TEST_CVT == CVT_SD2SI || TEST_CVT == CVT_TSD2SI
int main(void) {
    const double src = -2.75;
    int32_t out32;
    int64_t out64;
    uint32_t original = use_round_to_nearest();
    TEST_START(TEST_NAME);
    __asm__ volatile("vmovsd %2, %%xmm0\n\t" TEST_INSN " %%xmm0, %0\n\t"
                     TEST_INSN " %%xmm0, %1"
                     : "=r"(out32), "=r"(out64) : "m"(src) : "xmm0");
    restore_mxcsr(original);
#if TEST_CVT == CVT_TSD2SI
    TEST_ASSERT(out32 == -2 && out64 == -2, TEST_NAME " truncates 32/64-bit");
#else
    TEST_ASSERT(out32 == -3 && out64 == -3, TEST_NAME " rounds 32/64-bit");
#endif
    TEST_END();
}
#elif TEST_CVT == CVT_SS2SI || TEST_CVT == CVT_TSS2SI
int main(void) {
    const float src = 2.75f;
    int32_t out32;
    int64_t out64;
    uint32_t original = use_round_to_nearest();
    TEST_START(TEST_NAME);
    __asm__ volatile("vmovss %2, %%xmm0\n\t" TEST_INSN " %%xmm0, %0\n\t"
                     TEST_INSN " %%xmm0, %1"
                     : "=r"(out32), "=r"(out64) : "m"(src) : "xmm0");
    restore_mxcsr(original);
#if TEST_CVT == CVT_TSS2SI
    TEST_ASSERT(out32 == 2 && out64 == 2, TEST_NAME " truncates 32/64-bit");
#else
    TEST_ASSERT(out32 == 3 && out64 == 3, TEST_NAME " rounds 32/64-bit");
#endif
    TEST_END();
}
#elif TEST_CVT == CVT_SD2SS
int main(void) {
    xmm_t preserved = { .u32 = {0, 0x11112222, 0x33334444, 0x55556666} }, out;
    double src = 0x1.000001p+0;
    TEST_START("VCVTSD2SS");
    __asm__ volatile("movdqu %1, %%xmm0\n\tvcvtsd2ss %2, %%xmm0, %%xmm1\n\t"
                     "movdqu %%xmm1, %0"
                     : "=m"(out) : "m"(preserved), "m"(src) : "xmm0", "xmm1");
    TEST_ASSERT(out.f32[0] == (float)src, "VCVTSD2SS low result");
    for (int i = 1; i < 4; i++) TEST_ASSERT(out.u32[i] == preserved.u32[i], "VCVTSD2SS lane %d", i);
    TEST_END();
}
#elif TEST_CVT == CVT_SS2SD
int main(void) {
    xmm_t preserved = { .u64 = {0, UINT64_C(0x1122334455667788)} }, out;
    float src = 0x1.000002p+0f;
    TEST_START("VCVTSS2SD");
    __asm__ volatile("movdqu %1, %%xmm0\n\tvcvtss2sd %2, %%xmm0, %%xmm1\n\t"
                     "movdqu %%xmm1, %0"
                     : "=m"(out) : "m"(preserved), "m"(src) : "xmm0", "xmm1");
    TEST_ASSERT(out.f64[0] == (double)src, "VCVTSS2SD low result");
    TEST_ASSERT(out.u64[1] == preserved.u64[1], "VCVTSS2SD preserves upper qword");
    TEST_END();
}
#elif TEST_CVT == CVT_SI2SD
int main(void) {
    xmm_t preserved = { .u64 = {0, UINT64_C(0x1122334455667788)} }, out;
    int32_t src32 = INT32_MIN;
    int64_t src64 = INT64_C(9007199254740991);
    TEST_START("VCVTSI2SD");
    __asm__ volatile("movdqu %1, %%xmm0\n\tvcvtsi2sd %2, %%xmm0, %%xmm1\n\t"
                     "movdqu %%xmm1, %0"
                     : "=m"(out) : "m"(preserved), "r"(src32) : "xmm0", "xmm1");
    TEST_ASSERT(out.f64[0] == (double)src32 && out.u64[1] == preserved.u64[1],
                "VCVTSI2SD 32-bit source and preserved upper");
    __asm__ volatile("vcvtsi2sd %1, %%xmm0, %%xmm1\n\tvmovsd %%xmm1, %0"
                     : "=m"(out.f64[0]) : "r"(src64) : "xmm0", "xmm1");
    TEST_ASSERT(out.f64[0] == (double)src64, "VCVTSI2SD 64-bit source");
    TEST_END();
}
#elif TEST_CVT == CVT_SI2SS
int main(void) {
    xmm_t preserved = { .u32 = {0, 0x11112222, 0x33334444, 0x55556666} }, out;
    int32_t src32 = 16777217;
    int64_t src64 = INT64_C(1099511627776);
    TEST_START("VCVTSI2SS");
    __asm__ volatile("movdqu %1, %%xmm0\n\tvcvtsi2ss %2, %%xmm0, %%xmm1\n\t"
                     "movdqu %%xmm1, %0"
                     : "=m"(out) : "m"(preserved), "r"(src32) : "xmm0", "xmm1");
    TEST_ASSERT(out.f32[0] == (float)src32, "VCVTSI2SS 32-bit rounding");
    for (int i = 1; i < 4; i++) TEST_ASSERT(out.u32[i] == preserved.u32[i], "VCVTSI2SS lane %d", i);
    __asm__ volatile("vcvtsi2ss %1, %%xmm0, %%xmm1\n\tvmovss %%xmm1, %0"
                     : "=m"(out.f32[0]) : "r"(src64) : "xmm0", "xmm1");
    TEST_ASSERT(out.f32[0] == (float)src64, "VCVTSI2SS 64-bit source");
    TEST_END();
}
#else
#error "Unknown TEST_CVT"
#endif

#endif
