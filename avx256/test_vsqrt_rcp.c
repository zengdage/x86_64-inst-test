/*
 * Test VSQRTPD/VSQRTPS/VSQRTSD/VSQRTSS/VRCPPS/VRCPSS/VRSQRTPS/VRSQRTSS
 * Square root, reciprocal, and reciprocal square root instructions
 * Compile: gcc -o test_vsqrt_rcp avx256/test_vsqrt_rcp.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}
#else
#define check_avx() 1
#endif

static void test_vsqrtpd(void) {
    TEST_START("VSQRTPD (256-bit)");
    double a[4] = {4.0, 9.0, 16.0, 25.0};
    double r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vsqrtpd %%ymm0, %%ymm1\n\t"
        "vmovupd %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r[0] == 2.0, "vsqrtpd r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 3.0, "vsqrtpd r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == 4.0, "vsqrtpd r[2]=%f", r[2]);
    TEST_ASSERT(r[3] == 5.0, "vsqrtpd r[3]=%f", r[3]);
    /* mem form */
    double r2[4];
    __asm__ volatile(
        "vsqrtpd %1, %%ymm1\n\t"
        "vmovupd %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(r2[0] == 2.0, "vsqrtpd mem r[0]=%f", r2[0]);
}

static void test_vsqrtps(void) {
    TEST_START("VSQRTPS (256-bit)");
    float a[8] = {4,9,16,25,36,49,64,81};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vsqrtps %%ymm0, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r[0] == 2.0f, "vsqrtps r[0]=%f", r[0]);
    TEST_ASSERT(r[3] == 5.0f, "vsqrtps r[3]=%f", r[3]);
    TEST_ASSERT(r[7] == 9.0f, "vsqrtps r[7]=%f", r[7]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vsqrtps %1, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(r2[0] == 2.0f, "vsqrtps mem r[0]=%f", r2[0]);
}

static void test_vsqrtsd(void) {
    TEST_START("VSQRTSD (scalar double)");
    double a = 9.0, r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vsqrtsd %%xmm0, %%xmm0, %%xmm1\n\t"
        "vmovsd %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == 3.0, "vsqrtsd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vsqrtsd %1, %%xmm0, %%xmm1\n\t"
        "vmovsd %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == 3.0, "vsqrtsd mem r=%f", r);
}

static void test_vsqrtss(void) {
    TEST_START("VSQRTSS (scalar float)");
    float a = 16.0f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vsqrtss %%xmm0, %%xmm0, %%xmm1\n\t"
        "vmovss %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == 4.0f, "vsqrtss r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vsqrtss %1, %%xmm0, %%xmm1\n\t"
        "vmovss %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == 4.0f, "vsqrtss mem r=%f", r);
}

static void test_vrcpps(void) {
    TEST_START("VRCPPS (256-bit approx reciprocal)");
    float a[8] = {1.0f,2.0f,4.0f,8.0f,1.0f,2.0f,4.0f,8.0f};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vrcpps %%ymm0, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    /* approx: within 1.5*2^-12 relative error */
    TEST_ASSERT(fabsf(r[0] - 1.0f) < 0.001f, "vrcpps r[0]=%f", r[0]);
    TEST_ASSERT(fabsf(r[1] - 0.5f) < 0.001f, "vrcpps r[1]=%f", r[1]);
    TEST_ASSERT(fabsf(r[2] - 0.25f) < 0.001f, "vrcpps r[2]=%f", r[2]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vrcpps %1, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(fabsf(r2[0] - 1.0f) < 0.001f, "vrcpps mem r[0]=%f", r2[0]);
}

static void test_vrcpss(void) {
    TEST_START("VRCPSS (scalar float approx reciprocal)");
    float a = 4.0f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vrcpss %%xmm0, %%xmm0, %%xmm1\n\t"
        "vmovss %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(fabsf(r - 0.25f) < 0.001f, "vrcpss r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vrcpss %1, %%xmm0, %%xmm1\n\t"
        "vmovss %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(fabsf(r - 0.25f) < 0.001f, "vrcpss mem r=%f", r);
}

static void test_vrsqrtps(void) {
    TEST_START("VRSQRTPS (256-bit approx reciprocal sqrt)");
    float a[8] = {4,4,4,4,16,16,16,16};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vrsqrtps %%ymm0, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    /* 1/sqrt(4)=0.5, 1/sqrt(16)=0.25 */
    TEST_ASSERT(fabsf(r[0] - 0.5f) < 0.001f, "vrsqrtps r[0]=%f", r[0]);
    TEST_ASSERT(fabsf(r[4] - 0.25f) < 0.001f, "vrsqrtps r[4]=%f", r[4]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vrsqrtps %1, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]) : "ymm1"
    );
    TEST_ASSERT(fabsf(r2[0] - 0.5f) < 0.001f, "vrsqrtps mem r[0]=%f", r2[0]);
}

static void test_vrsqrtss(void) {
    TEST_START("VRSQRTSS (scalar float approx reciprocal sqrt)");
    float a = 4.0f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vrsqrtss %%xmm0, %%xmm0, %%xmm1\n\t"
        "vmovss %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(fabsf(r - 0.5f) < 0.001f, "vrsqrtss r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vrsqrtss %1, %%xmm0, %%xmm1\n\t"
        "vmovss %%xmm1, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0","xmm1"
    );
    TEST_ASSERT(fabsf(r - 0.5f) < 0.001f, "vrsqrtss mem r=%f", r);
}

static void test_vsqrt_rcp_boundaries(void) {
    double ad[4] = {0.0, -0.0, INFINITY, -1.0};
    double rd[4];
    float af[8] = {0.0f, -0.0f, INFINITY, -INFINITY,
                   NAN, -1.0f, FLT_MAX, 1.0f};
    float rf[8];

    __asm__ volatile (
        "vsqrtpd %1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0"
        : "=m"(rd[0]) : "m"(ad[0]) : "ymm0"
    );
    TEST_ASSERT(rd[0] == 0.0 && !signbit(rd[0]), "vsqrtpd +0 preserves sign");
    TEST_ASSERT(rd[1] == 0.0 && signbit(rd[1]), "vsqrtpd -0 preserves sign");
    TEST_ASSERT(isinf(rd[2]) && rd[2] > 0.0, "vsqrtpd +inf = +inf");
    TEST_ASSERT(isnan(rd[3]), "vsqrtpd negative finite = NaN");

    __asm__ volatile (
        "vrcpps %1, %%ymm0\n\t"
        "vmovups %%ymm0, %0"
        : "=m"(rf[0]) : "m"(af[0]) : "ymm0"
    );
    TEST_ASSERT(isinf(rf[0]) && rf[0] > 0.0f, "vrcpps +0 = +inf");
    TEST_ASSERT(isinf(rf[1]) && rf[1] < 0.0f, "vrcpps -0 = -inf");
    TEST_ASSERT(rf[2] == 0.0f && !signbit(rf[2]), "vrcpps +inf = +0");
    TEST_ASSERT(rf[3] == 0.0f && signbit(rf[3]), "vrcpps -inf = -0");
    TEST_ASSERT(isnan(rf[4]), "vrcpps NaN propagates NaN");
    TEST_ASSERT(rf[5] < 0.0f, "vrcpps negative input preserves result sign");

    __asm__ volatile (
        "vrsqrtps %1, %%ymm0\n\t"
        "vmovups %%ymm0, %0"
        : "=m"(rf[0]) : "m"(af[0]) : "ymm0"
    );
    TEST_ASSERT(isinf(rf[0]) && rf[0] > 0.0f, "vrsqrtps +0 = +inf");
    TEST_ASSERT(isinf(rf[1]) && rf[1] < 0.0f, "vrsqrtps -0 = -inf");
    TEST_ASSERT(rf[2] == 0.0f && !signbit(rf[2]), "vrsqrtps +inf = +0");
    TEST_ASSERT(isnan(rf[3]), "vrsqrtps -inf = NaN");
    TEST_ASSERT(isnan(rf[4]) && isnan(rf[5]), "vrsqrtps NaN and negative finite = NaN");
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vsqrtpd();
    test_vsqrtps();
    test_vsqrtsd();
    test_vsqrtss();
    test_vrcpps();
    test_vrcpss();
    test_vrsqrtps();
    test_vrsqrtss();
    test_vsqrt_rcp_boundaries();
    TEST_END();
}
