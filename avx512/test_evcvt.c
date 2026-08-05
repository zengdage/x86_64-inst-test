/*
 * Test VCVTPS2PD/VCVTPD2PS/VCVTDQ2PD/VCVTDQ2PS/VCVTPD2DQ/VCVTPS2DQ with zmm (AVX-512F).
 * Packed floating-point conversion instructions.
 *
 * Compile: gcc -o test_evcvt avx512/test_evcvt.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include "../common.h"

typedef union {
    long long    i64[8];
    int          i32[16];
    short        i16[32];
    signed char  i8[64];
    unsigned long long u64[8];
    unsigned int       u32[16];
    unsigned short     u16[32];
    unsigned char      u8[64];
    float  f32[16];
    double f64[8];
} zmm_t __attribute__((aligned(64)));

static void test_conversion_boundaries(void) {
    zmm_t src, dst;
    ymm_t ydst;

    const float ps_values[16] = {
        NAN, INFINITY, -INFINITY, 2147483648.0f,
        -2147483904.0f, 0.5f, -0.5f, 1.5f,
        2.5f, -1.5f, -2.5f, 0.0f,
        -0.0f, 2147483520.0f, -2147483648.0f, 42.0f
    };
    const uint32_t ps_expected[16] = {
        0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u,
        0x80000000u, 0u, 0u, 2u,
        2u, (uint32_t)-2, (uint32_t)-2, 0u,
        0u, 0x7fffff80u, 0x80000000u, 42u
    };
    memcpy(src.f32, ps_values, sizeof(ps_values));
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vcvtps2dq %%zmm0, %%zmm1\n\t"
        "vmovdqa32 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0", "zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == ps_expected[i],
            "VCVTPS2DQ boundary lane %d: 0x%08x != 0x%08x", i, dst.u32[i], ps_expected[i]);

    const double pd_values[8] = {
        NAN, INFINITY, -INFINITY, 2147483648.0,
        -2147483649.0, 0.5, 1.5, 2.5
    };
    const uint32_t pd_expected[8] = {
        0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u,
        0x80000000u, 0u, 2u, 2u
    };
    memcpy(src.f64, pd_values, sizeof(pd_values));
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vcvtpd2dq %%zmm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0"
        : "=m"(ydst) : "m"(src) : "zmm0", "ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(ydst.u32[i] == pd_expected[i],
            "VCVTPD2DQ boundary lane %d: 0x%08x != 0x%08x", i, ydst.u32[i], pd_expected[i]);

    const double pd_round_values[8] = {0.5, -0.5, 1.5, 2.5, -1.5, -2.5, 3.5, -3.5};
    const int32_t pd_round_expected[8] = {0, 0, 2, 2, -2, -2, 4, -4};
    memcpy(src.f64, pd_round_values, sizeof(pd_round_values));
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vcvtpd2dq %%zmm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0"
        : "=m"(ydst) : "m"(src) : "zmm0", "ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(ydst.i32[i] == pd_round_expected[i],
            "VCVTPD2DQ ties-to-even lane %d: %d != %d", i, ydst.i32[i], pd_round_expected[i]);

    src.f64[0] = NAN;
    src.f64[1] = INFINITY;
    src.f64[2] = -INFINITY;
    src.f64[3] = DBL_MAX;
    src.f64[4] = 0.0;
    src.f64[5] = -0.0;
    src.f64[6] = 0x1p-149;
    src.f64[7] = DBL_MIN;
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vcvtpd2ps %%zmm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(ydst) : "m"(src) : "zmm0", "ymm1"
    );
    TEST_ASSERT(IS_QNAN(ydst.f32[0]), "VCVTPD2PS QNaN propagation");
    TEST_ASSERT(isinf(ydst.f32[1]) && !signbit(ydst.f32[1]), "VCVTPD2PS +Inf");
    TEST_ASSERT(isinf(ydst.f32[2]) && signbit(ydst.f32[2]), "VCVTPD2PS -Inf");
    TEST_ASSERT(isinf(ydst.f32[3]) && !signbit(ydst.f32[3]), "VCVTPD2PS overflow to +Inf");
    TEST_ASSERT(ydst.f32[4] == 0.0f && !signbit(ydst.f32[4]), "VCVTPD2PS +0 sign");
    TEST_ASSERT(ydst.f32[5] == 0.0f && signbit(ydst.f32[5]), "VCVTPD2PS -0 sign");
    TEST_ASSERT(ydst.f32[6] == 0x1p-149f, "VCVTPD2PS exact minimum subnormal");
    TEST_ASSERT(ydst.f32[7] == 0.0f, "VCVTPD2PS underflow to zero");

    for (int i = 0; i < 16; i++) src.i32[i] = 0;
    src.i32[0] = INT32_MIN;
    src.i32[1] = INT32_MAX;
    src.i32[2] = 16777215;
    src.i32[3] = 16777217;
    __asm__ volatile (
        "vmovdqa32 %1, %%zmm0\n\t"
        "vcvtdq2ps %%zmm0, %%zmm1\n\t"
        "vmovaps %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0", "zmm1"
    );
    TEST_ASSERT(dst.f32[0] == (float)INT32_MIN, "VCVTDQ2PS INT32_MIN");
    TEST_ASSERT(dst.f32[1] == (float)INT32_MAX, "VCVTDQ2PS INT32_MAX rounding");
    TEST_ASSERT(dst.f32[2] == 16777215.0f, "VCVTDQ2PS last consecutive integer");
    TEST_ASSERT(dst.f32[3] == 16777216.0f, "VCVTDQ2PS ties-to-even boundary");
}

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 16) & 1;
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVCVT (VCVTPS2PD/VCVTPD2PS/VCVTDQ2PD/VCVTDQ2PS/VCVTPD2DQ/VCVTPS2DQ zmm)");

    zmm_t src, dst;

    /* VCVTPS2PD: 8 floats (ymm) -> 8 doubles (zmm) */
    ymm_t ysrc;
    for (int i = 0; i < 8; i++) ysrc.f32[i] = (float)(i + 1) * 1.5f;
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vcvtps2pd %%ymm0, %%zmm1\n\t"
        "vmovapd %%zmm1, %0"
        : "=m"(dst) : "m"(ysrc) : "ymm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.f64[i] == (double)ysrc.f32[i],
            "VCVTPS2PD lane %d: %f != %f", i, dst.f64[i], (double)ysrc.f32[i]);

    /* VCVTPD2PS: 8 doubles (zmm) -> 8 floats (ymm) */
    for (int i = 0; i < 8; i++) src.f64[i] = (double)(i + 1) * 2.0;
    ymm_t ydst;
    memset(&ydst, 0, sizeof(ydst));
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vcvtpd2ps %%zmm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(ydst) : "m"(src) : "zmm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(ydst.f32[i] == (float)src.f64[i],
            "VCVTPD2PS lane %d: %f != %f", i, ydst.f32[i], (float)src.f64[i]);

    /* VCVTDQ2PS: 16 int32 -> 16 float32 (zmm) */
    for (int i = 0; i < 16; i++) src.i32[i] = i * 3 - 10;
    __asm__ volatile (
        "vmovdqa32 %1, %%zmm0\n\t"
        "vcvtdq2ps %%zmm0, %%zmm1\n\t"
        "vmovaps %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == (float)src.i32[i],
            "VCVTDQ2PS lane %d: %f != %f", i, dst.f32[i], (float)src.i32[i]);

    /* VCVTPS2DQ: 16 float32 -> 16 int32 (zmm) */
    for (int i = 0; i < 16; i++) src.f32[i] = (float)(i * 2 - 5);
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vcvtps2dq %%zmm0, %%zmm1\n\t"
        "vmovdqa32 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i32[i] == (int)src.f32[i],
            "VCVTPS2DQ lane %d: %d != %d", i, dst.i32[i], (int)src.f32[i]);

    /* VCVTDQ2PD: 8 int32 (ymm lower) -> 8 double (zmm) */
    ymm_t yisrc;
    for (int i = 0; i < 8; i++) yisrc.i32[i] = i * 5 - 10;
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vcvtdq2pd %%ymm0, %%zmm1\n\t"
        "vmovapd %%zmm1, %0"
        : "=m"(dst) : "m"(yisrc) : "ymm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.f64[i] == (double)yisrc.i32[i],
            "VCVTDQ2PD lane %d: %f != %f", i, dst.f64[i], (double)yisrc.i32[i]);

    /* VCVTPD2DQ: 8 double (zmm) -> 8 int32 (ymm) */
    for (int i = 0; i < 8; i++) src.f64[i] = (double)(i * 3 - 7);
    memset(&ydst, 0, sizeof(ydst));
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vcvtpd2dq %%zmm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0"
        : "=m"(ydst) : "m"(src) : "zmm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(ydst.i32[i] == (int)src.f64[i],
            "VCVTPD2DQ lane %d: %d != %d", i, ydst.i32[i], (int)src.f64[i]);

    test_conversion_boundaries();

    TEST_END();
}
