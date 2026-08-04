/*
 * Test VCVTDQ2PD/VCVTDQ2PS/VCVTPD2DQ/VCVTPD2PS/VCVTPS2DQ/VCVTPS2PD/
 *      VCVTSD2SI/VCVTSD2SS/VCVTSI2SD/VCVTSI2SS/VCVTSS2SD/VCVTSS2SI/
 *      VCVTTPD2DQ/VCVTTPS2DQ/VCVTTSD2SI/VCVTTSS2SI
 * Packed and scalar float/int conversion instructions
 * Compile: gcc -o test_vcvt avx256/test_vcvt.c -O0 -mavx2 -mfma
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(7),"c"(0));
    return (ebx >> 5) & 1;
}
#else
#define check_avx2() 1
#endif

static void test_vcvtdq2pd(void) {
    TEST_START("VCVTDQ2PD (128-bit: 2 int32->double)");
    int32_t a[4] = {1, -2, 3, -4};
    double r[2];
    __asm__ volatile(
        "vmovdqu %1, %%xmm0\n\t"
        "vcvtdq2pd %%xmm0, %%xmm1\n\t"
        "vmovupd %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "xmm0","xmm1"
    );
    TEST_ASSERT(r[0] == 1.0,  "vcvtdq2pd r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == -2.0, "vcvtdq2pd r[1]=%f", r[1]);
    /* mem form */
    double r2[2];
    __asm__ volatile(
        "vcvtdq2pd %1, %%xmm1\n\t"
        "vmovupd %%xmm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "xmm1"
    );
    TEST_ASSERT(r2[0] == 1.0, "vcvtdq2pd mem r[0]=%f", r2[0]);
}

static void test_vcvtdq2ps(void) {
    TEST_START("VCVTDQ2PS (256-bit: 8 int32->float)");
    int32_t a[8] = {1,-2,3,-4,5,-6,7,-8};
    float r[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vcvtdq2ps %%ymm0, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r[0] == 1.0f,  "vcvtdq2ps r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == -2.0f, "vcvtdq2ps r[1]=%f", r[1]);
    TEST_ASSERT(r[7] == -8.0f, "vcvtdq2ps r[7]=%f", r[7]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vcvtdq2ps %1, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(r2[0] == 1.0f, "vcvtdq2ps mem r[0]=%f", r2[0]);
}

static void test_vcvtpd2dq(void) {
    TEST_START("VCVTPD2DQ (256->128: 4 double->int32)");
    double a[4] = {1.7, -2.3, 3.5, -4.5};
    int32_t r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vcvtpd2dq %%ymm0, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","xmm1"
    );
    TEST_ASSERT(r[0] == 2,  "vcvtpd2dq r[0]=%d (round-nearest)", r[0]);
    TEST_ASSERT(r[1] == -2, "vcvtpd2dq r[1]=%d", r[1]);
    /* mem form */
    int32_t r2[4];
    __asm__ volatile(
        "vcvtpd2dqy %1, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "xmm1"
    );
    TEST_ASSERT(r2[0] == 2, "vcvtpd2dq mem r[0]=%d", r2[0]);
}

static void test_vcvtpd2ps(void) {
    TEST_START("VCVTPD2PS (256->128: 4 double->float)");
    double a[4] = {1.5, -2.5, 3.0, -4.0};
    float r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vcvtpd2ps %%ymm0, %%xmm1\n\t"
        "vmovups %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","xmm1"
    );
    TEST_ASSERT(r[0] == 1.5f,  "vcvtpd2ps r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == -2.5f, "vcvtpd2ps r[1]=%f", r[1]);
    /* mem form */
    float r2[4];
    __asm__ volatile(
        "vcvtpd2psy %1, %%xmm1\n\t"
        "vmovups %%xmm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "xmm1"
    );
    TEST_ASSERT(r2[0] == 1.5f, "vcvtpd2ps mem r[0]=%f", r2[0]);
}

static void test_vcvtps2dq(void) {
    TEST_START("VCVTPS2DQ (256-bit: 8 float->int32)");
    float a[8] = {1.7f,-2.3f,3.5f,-4.5f,5.0f,-6.0f,7.9f,-8.1f};
    int32_t r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vcvtps2dq %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r[0] == 2,  "vcvtps2dq r[0]=%d", r[0]);
    TEST_ASSERT(r[1] == -2, "vcvtps2dq r[1]=%d", r[1]);
    TEST_ASSERT(r[4] == 5,  "vcvtps2dq r[4]=%d", r[4]);
    /* mem form */
    int32_t r2[8];
    __asm__ volatile(
        "vcvtps2dq %1, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(r2[0] == 2, "vcvtps2dq mem r[0]=%d", r2[0]);
}

static void test_vcvtps2pd(void) {
    TEST_START("VCVTPS2PD (128->256: 4 float->double)");
    float a[4] = {1.5f, -2.5f, 3.0f, -4.0f};
    double r[4];
    __asm__ volatile(
        "vmovups %1, %%xmm0\n\t"
        "vcvtps2pd %%xmm0, %%ymm1\n\t"
        "vmovupd %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "xmm0","ymm1"
    );
    TEST_ASSERT(r[0] == 1.5,  "vcvtps2pd r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == -2.5, "vcvtps2pd r[1]=%f", r[1]);
    /* mem form */
    double r2[4];
    __asm__ volatile(
        "vcvtps2pd %1, %%ymm1\n\t"
        "vmovupd %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(r2[0] == 1.5, "vcvtps2pd mem r[0]=%f", r2[0]);
}

static void test_vcvtsd2si(void) {
    TEST_START("VCVTSD2SI (scalar double->int32/64)");
    double a = 3.7;
    int32_t r32;
    int64_t r64;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vcvtsd2si %%xmm0, %0\n\t"
        : "=r"(r32) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r32 == 4, "vcvtsd2si r32=%d", r32);
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vcvtsd2siq %%xmm0, %0\n\t"
        : "=r"(r64) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r64 == 4, "vcvtsd2si r64=%lld", (long long)r64);
}

static void test_vcvtsd2ss(void) {
    TEST_START("VCVTSD2SS (scalar double->float)");
    double a = 1.5;
    float r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %1, %%xmm1\n\t"
        "vcvtsd2ss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 1.5f, "vcvtsd2ss r=%f", r);
}

static void test_vcvtsi2sd(void) {
    TEST_START("VCVTSI2SD (int32/64->scalar double)");
    int32_t i32 = 42;
    double r;
    __asm__ volatile(
        "vxorpd %%xmm0, %%xmm0, %%xmm0\n\t"
        "vcvtsi2sd %1, %%xmm0, %%xmm1\n\t"
        "vmovsd %%xmm1, %0\n\t"
        : "=m"(r) : "m"(i32) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == 42.0, "vcvtsi2sd r=%f", r);
}

static void test_vcvtsi2ss(void) {
    TEST_START("VCVTSI2SS (int32->scalar float)");
    int32_t i = 7;
    float r;
    __asm__ volatile(
        "vxorps %%xmm0, %%xmm0, %%xmm0\n\t"
        "vcvtsi2ss %1, %%xmm0, %%xmm1\n\t"
        "vmovss %%xmm1, %0\n\t"
        : "=m"(r) : "m"(i) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == 7.0f, "vcvtsi2ss r=%f", r);
}

static void test_vcvtss2sd(void) {
    TEST_START("VCVTSS2SD (scalar float->double)");
    float a = 2.5f;
    double r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %1, %%xmm1\n\t"
        "vcvtss2sd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 2.5, "vcvtss2sd r=%f", r);
}

static void test_vcvtss2si(void) {
    TEST_START("VCVTSS2SI (scalar float->int32)");
    float a = 3.7f;
    int32_t r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vcvtss2si %%xmm0, %0\n\t"
        : "=r"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r == 4, "vcvtss2si r=%d", r);
}

static void test_vcvttpd2dq(void) {
    TEST_START("VCVTTPD2DQ (256->128: 4 double->int32 truncate)");
    double a[4] = {1.9, -2.9, 3.1, -4.1};
    int32_t r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vcvttpd2dq %%ymm0, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","xmm1"
    );
    TEST_ASSERT(r[0] == 1,  "vcvttpd2dq r[0]=%d", r[0]);
    TEST_ASSERT(r[1] == -2, "vcvttpd2dq r[1]=%d", r[1]);
    TEST_ASSERT(r[2] == 3,  "vcvttpd2dq r[2]=%d", r[2]);
    TEST_ASSERT(r[3] == -4, "vcvttpd2dq r[3]=%d", r[3]);
    /* mem form */
    int32_t r2[4];
    __asm__ volatile(
        "vcvttpd2dqy %1, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "xmm1"
    );
    TEST_ASSERT(r2[0] == 1, "vcvttpd2dq mem r[0]=%d", r2[0]);
}

static void test_vcvttps2dq(void) {
    TEST_START("VCVTTPS2DQ (256-bit: 8 float->int32 truncate)");
    float a[8] = {1.9f,-2.9f,3.1f,-4.1f,5.9f,-6.9f,7.1f,-8.1f};
    int32_t r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vcvttps2dq %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r[0] == 1,  "vcvttps2dq r[0]=%d", r[0]);
    TEST_ASSERT(r[1] == -2, "vcvttps2dq r[1]=%d", r[1]);
    TEST_ASSERT(r[4] == 5,  "vcvttps2dq r[4]=%d", r[4]);
    /* mem form */
    int32_t r2[8];
    __asm__ volatile(
        "vcvttps2dq %1, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(r2[0] == 1, "vcvttps2dq mem r[0]=%d", r2[0]);
}

static void test_vcvttsd2si(void) {
    TEST_START("VCVTTSD2SI (scalar double->int32 truncate)");
    double a = 3.9;
    int32_t r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vcvttsd2si %%xmm0, %0\n\t"
        : "=r"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r == 3, "vcvttsd2si r=%d", r);
    double b = -3.9;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vcvttsd2si %%xmm0, %0\n\t"
        : "=r"(r) : "m"(b) : "xmm0"
    );
    TEST_ASSERT(r == -3, "vcvttsd2si neg r=%d", r);
}

static void test_vcvttss2si(void) {
    TEST_START("VCVTTSS2SI (scalar float->int32 truncate)");
    float a = 3.9f;
    int32_t r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vcvttss2si %%xmm0, %0\n\t"
        : "=r"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r == 3, "vcvttss2si r=%d", r);
    float b = -3.9f;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vcvttss2si %%xmm0, %0\n\t"
        : "=r"(r) : "m"(b) : "xmm0"
    );
    TEST_ASSERT(r == -3, "vcvttss2si neg r=%d", r);
}

static void test_vcvt_boundaries(void) {
    int32_t ints[4] = {INT32_MIN, INT32_MAX, 0, -1};
    double doubles[4];
    double invalid_d[4] = {NAN, INFINITY, 2147483648.0, -2147483649.0};
    float invalid_f[8] = {NAN, INFINITY, -INFINITY, 2147483648.0f,
                          -2147483904.0f, 0.0f, -0.0f, 1.0f};
    int32_t out32[8];
    int64_t out64;

    __asm__ volatile (
        "vcvtdq2pd %1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0"
        : "=m"(doubles[0]) : "m"(ints[0]) : "ymm0"
    );
    TEST_ASSERT(doubles[0] == (double)INT32_MIN, "vcvtdq2pd INT32_MIN exact");
    TEST_ASSERT(doubles[1] == (double)INT32_MAX, "vcvtdq2pd INT32_MAX exact");

    __asm__ volatile (
        "vcvttpd2dqy %1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(out32[0]) : "m"(invalid_d[0]) : "xmm0"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT((uint32_t)out32[i] == UINT32_C(0x80000000),
                    "vcvttpd2dq invalid lane %d: integer-indefinite", i);

    __asm__ volatile (
        "vcvttps2dq %1, %%ymm0\n\t"
        "vmovdqu %%ymm0, %0"
        : "=m"(out32[0]) : "m"(invalid_f[0]) : "ymm0"
    );
    for (int i = 0; i < 5; i++)
        TEST_ASSERT((uint32_t)out32[i] == UINT32_C(0x80000000),
                    "vcvttps2dq invalid lane %d: integer-indefinite", i);
    TEST_ASSERT(out32[5] == 0 && out32[6] == 0 && out32[7] == 1,
                "vcvttps2dq zero and one boundaries");

    {
        double nan_value = NAN;
        __asm__ volatile (
            "vcvttsd2siq %1, %0"
            : "=r"(out64) : "m"(nan_value)
        );
        TEST_ASSERT((uint64_t)out64 == UINT64_C(0x8000000000000000),
                    "vcvttsd2siq NaN: integer-indefinite");
    }
}

static void test_vcvt_rounding_flags_and_scalar_merge(void) {
    float ps[8] = {1.5f, -1.5f, 2.5f, -2.5f, 1.9f, -1.1f, 0.0f, -0.0f};
    double pd[4] = {1.5, -1.5, 2.5, -2.5};
    int32_t out[8];
    const int32_t expected_ps[4][8] = {
        {2,-2,2,-2,2,-1,0,0}, {1,-2,2,-3,1,-2,0,0},
        {2,-1,3,-2,2,-1,0,0}, {1,-1,2,-2,1,-1,0,0}
    };
    const int32_t expected_pd[4][4] = {
        {2,-2,2,-2}, {1,-2,2,-3}, {2,-1,3,-2}, {1,-1,2,-2}
    };
    uint32_t saved, csr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    for (uint32_t mode = 0; mode < 4; mode++) {
        csr = (saved & ~((UINT32_C(3) << 13) | UINT32_C(0x3f))) | (mode << 13);
        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        __asm__ volatile("vcvtps2dq %1,%%ymm0\n\tvmovdqu %%ymm0,%0"
            : "=m"(out[0]) : "m"(ps[0]) : "ymm0");
        for (int lane = 0; lane < 8; lane++)
            TEST_ASSERT(out[lane] == expected_ps[mode][lane],
                        "vcvtps2dq MXCSR mode %u lane %d", mode, lane);
        __asm__ volatile("stmxcsr %0" : "=m"(csr));
        TEST_ASSERT(csr & (1u << 5), "vcvtps2dq inexact sets precision mode %u", mode);

        csr = (saved & ~((UINT32_C(3) << 13) | UINT32_C(0x3f))) | (mode << 13);
        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        __asm__ volatile("vcvtpd2dqy %1,%%xmm0\n\tvmovdqu %%xmm0,%0"
            : "=m"(out[0]) : "m"(pd[0]) : "xmm0");
        for (int lane = 0; lane < 4; lane++)
            TEST_ASSERT(out[lane] == expected_pd[mode][lane],
                        "vcvtpd2dq MXCSR mode %u lane %d", mode, lane);
    }

    ymm_t initial, result;
    xmm_t merge = { .u32 = {UINT32_C(0xaaaaaaaa), UINT32_C(0x80000000),
                             UINT32_C(0x7fc12345), UINT32_C(0x00000001)} };
    double source_d = 1.5;
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile(
        "vmovdqu %1,%%ymm2\n\tvmovdqu %2,%%xmm0\n\t"
        "vcvtsd2ss %3,%%xmm0,%%xmm2\n\tvmovdqu %%ymm2,%0"
        : "=m"(result) : "m"(initial), "m"(merge), "m"(source_d)
        : "xmm0", "ymm2");
    TEST_ASSERT(result.u32[0] == UINT32_C(0x3fc00000), "vcvtsd2ss converted low lane");
    for (int lane = 1; lane < 4; lane++)
        TEST_ASSERT(result.u32[lane] == merge.u32[lane],
                    "vcvtsd2ss merge source upper XMM lane %d", lane);
    for (int lane = 4; lane < 8; lane++)
        TEST_ASSERT(result.u32[lane] == 0, "vcvtsd2ss clears upper YMM lane %d", lane);

    float source_f = -2.5f;
    __asm__ volatile(
        "vmovdqu %1,%%ymm2\n\tvmovdqu %2,%%xmm0\n\t"
        "vcvtss2sd %3,%%xmm0,%%xmm2\n\tvmovdqu %%ymm2,%0"
        : "=m"(result) : "m"(initial), "m"(merge), "m"(source_f)
        : "xmm0", "ymm2");
    TEST_ASSERT(result.u64[0] == UINT64_C(0xc004000000000000), "vcvtss2sd converted low lane");
    TEST_ASSERT(result.u64[1] == merge.u64[1], "vcvtss2sd upper qword from merge source");
    TEST_ASSERT(result.u64[2] == 0 && result.u64[3] == 0,
                "vcvtss2sd clears upper YMM");

    double nan_value = NAN;
    int64_t indefinite;
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("vcvttsd2siq %1,%0" : "=r"(indefinite) : "m"(nan_value));
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT((uint64_t)indefinite == UINT64_C(0x8000000000000000),
                "vcvttsd2siq NaN integer-indefinite");
    TEST_ASSERT(csr & 1u, "vcvttsd2siq NaN sets invalid flag");
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vcvtdq2pd();
    test_vcvtdq2ps();
    test_vcvtpd2dq();
    test_vcvtpd2ps();
    test_vcvtps2dq();
    test_vcvtps2pd();
    test_vcvtsd2si();
    test_vcvtsd2ss();
    test_vcvtsi2sd();
    test_vcvtsi2ss();
    test_vcvtss2sd();
    test_vcvtss2si();
    test_vcvttpd2dq();
    test_vcvttps2dq();
    test_vcvttsd2si();
    test_vcvttss2si();
    test_vcvt_boundaries();
    test_vcvt_rounding_flags_and_scalar_merge();
    TEST_END();
}
