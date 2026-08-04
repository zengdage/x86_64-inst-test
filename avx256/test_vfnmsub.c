/*
 * Test VFNMSUB132PS/VFNMSUB213PS/VFNMSUB231PS/PD + VFNMSUBSD/SS
 * FMA3 negated multiply-subtract: -(a*b) - c
 * Compile: gcc -o test_vfnmsub avx256/test_vfnmsub.c -O0 -mavx2 -mfma
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_fma(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 12) & 1;
}
#else
#define check_fma() 1
#endif

/* VFNMSUB132PS: dst = -(dst * src2) - src1 */
static void test_vfnmsub132ps(void) {
    TEST_START("VFNMSUB132PS (256-bit)");
    float a[8] = {2,2,2,2,2,2,2,2}; /* dst */
    float b[8] = {3,3,3,3,3,3,3,3}; /* src1 (subtracted) */
    float c[8] = {4,4,4,4,4,4,4,4}; /* src2 */
    float r[8];
    /* r = -(a*c) - b = -(2*4)-3 = -11 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vmovups %3, %%ymm2\n\t"
        "vfnmsub132ps %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == -11.0f, "vfnmsub132ps r[%d]=%f", i, r[i]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vfnmsub132ps %3, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r2[0] == -11.0f, "vfnmsub132ps mem r[0]=%f", r2[0]);
}

/* VFNMSUB213PS: dst = -(src1 * dst) - src2 */
static void test_vfnmsub213ps(void) {
    TEST_START("VFNMSUB213PS (256-bit)");
    float a[8] = {2,2,2,2,2,2,2,2}; /* dst */
    float b[8] = {3,3,3,3,3,3,3,3}; /* src1 */
    float c[8] = {5,5,5,5,5,5,5,5}; /* src2 (subtracted) */
    float r[8];
    /* r = -(b*a) - c = -(3*2)-5 = -11 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vmovups %3, %%ymm2\n\t"
        "vfnmsub213ps %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == -11.0f, "vfnmsub213ps r[%d]=%f", i, r[i]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vfnmsub213ps %3, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r2[0] == -11.0f, "vfnmsub213ps mem r[0]=%f", r2[0]);
}

/* VFNMSUB231PS: dst = -(src1 * src2) - dst */
static void test_vfnmsub231ps(void) {
    TEST_START("VFNMSUB231PS (256-bit)");
    float a[8] = {1,1,1,1,1,1,1,1}; /* dst (subtracted) */
    float b[8] = {3,3,3,3,3,3,3,3}; /* src1 */
    float c[8] = {4,4,4,4,4,4,4,4}; /* src2 */
    float r[8];
    /* r = -(b*c) - a = -(3*4)-1 = -13 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vmovups %3, %%ymm2\n\t"
        "vfnmsub231ps %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == -13.0f, "vfnmsub231ps r[%d]=%f", i, r[i]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vfnmsub231ps %3, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r2[0] == -13.0f, "vfnmsub231ps mem r[0]=%f", r2[0]);
}

/* VFNMSUB132PD/213PD/231PD (256-bit double) */
static void test_vfnmsub_pd(void) {
    TEST_START("VFNMSUB132PD/213PD/231PD (256-bit double)");
    double a[4] = {2,2,2,2};
    double b[4] = {3,3,3,3};
    double c[4] = {4,4,4,4};
    double r[4];
    /* 132: -(a*c)-b = -(2*4)-3 = -11 */
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmovupd %3, %%ymm2\n\t"
        "vfnmsub132pd %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == -11.0, "vfnmsub132pd r[%d]=%f", i, r[i]);
    /* 213: -(b*a)-c = -(3*2)-4 = -10 */
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmovupd %3, %%ymm2\n\t"
        "vfnmsub213pd %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == -10.0, "vfnmsub213pd r[%d]=%f", i, r[i]);
    /* 231: -(b*c)-a = -(3*4)-2 = -14 */
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmovupd %3, %%ymm2\n\t"
        "vfnmsub231pd %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == -14.0, "vfnmsub231pd r[%d]=%f", i, r[i]);
}

/* VFNMSUB132SD/213SD/231SD (scalar double) */
static void test_vfnmsub_sd(void) {
    TEST_START("VFNMSUB132SD/213SD/231SD (scalar double)");
    double a = 2.0, b = 3.0, c = 4.0, r;
    /* 132: -(a*c)-b = -11 */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %3, %%xmm2\n\t"
        "vfnmsub132sd %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -11.0, "vfnmsub132sd r=%f", r);
    /* 213: -(b*a)-c = -10 */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %3, %%xmm2\n\t"
        "vfnmsub213sd %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -10.0, "vfnmsub213sd r=%f", r);
    /* 231: -(b*c)-a = -14 */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %3, %%xmm2\n\t"
        "vfnmsub231sd %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -14.0, "vfnmsub231sd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vfnmsub132sd %3, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == -11.0, "vfnmsub132sd mem r=%f", r);
}

/* VFNMSUB132SS/213SS/231SS (scalar float) */
static void test_vfnmsub_ss(void) {
    TEST_START("VFNMSUB132SS/213SS/231SS (scalar float)");
    float a = 2.0f, b = 3.0f, c = 4.0f, r;
    /* 132: -(a*c)-b = -11 */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmovss %3, %%xmm2\n\t"
        "vfnmsub132ss %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovss %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -11.0f, "vfnmsub132ss r=%f", r);
    /* 213: -(b*a)-c = -10 */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmovss %3, %%xmm2\n\t"
        "vfnmsub213ss %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovss %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -10.0f, "vfnmsub213ss r=%f", r);
    /* 231: -(b*c)-a = -14 */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmovss %3, %%xmm2\n\t"
        "vfnmsub231ss %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovss %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -14.0f, "vfnmsub231ss r=%f", r);
}

static void test_vfnmsub_special_and_fused(void) {
    TEST_START("VFNMSUB special values and fused rounding");
    double a[4] = {0x1.0000000000001p+0, INFINITY, NAN, DBL_MAX};
    double subtrahend[4] = {-1.0, 1.0, 1.0, 0.0};
    double multiplier[4] = {0x1.fffffffffffffp-1, 0.0, 1.0, 2.0};
    double r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmovupd %3, %%ymm2\n\t"
        "vfnmsub132pd %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0"
        : "=m"(r) : "m"(a), "m"(subtrahend), "m"(multiplier) : "ymm0", "ymm1", "ymm2"
    );
    TEST_ASSERT(r[0] == -0x1.ffffffffffffep-54,
        "vfnmsub fused single-round result: %a", r[0]);
    volatile double rounded_product = a[0] * multiplier[0];
    TEST_ASSERT(-rounded_product - subtrahend[0] == 0.0,
        "vfnmsub discriminator requires ordinary multiply/subtract to cancel");
    TEST_ASSERT(isnan(r[1]), "vfnmsub Inf * 0 is NaN");
    TEST_ASSERT(isnan(r[2]), "vfnmsub NaN propagation");
    TEST_ASSERT(isinf(r[3]) && signbit(r[3]), "vfnmsub overflow is -Inf");

    float fa[8] = {0.0f, FLT_MIN, INFINITY, NAN, FLT_MAX, -0.0f, 1.0f, -1.0f};
    float fb[8] = {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, INFINITY, -INFINITY};
    float fc[8] = {2.0f, 0.5f, 0.0f, 1.0f, 2.0f, 2.0f, 1.0f, 1.0f};
    float fr[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vmovups %3, %%ymm2\n\t"
        "vfnmsub132ps %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0"
        : "=m"(fr) : "m"(fa), "m"(fb), "m"(fc) : "ymm0", "ymm1", "ymm2"
    );
    TEST_ASSERT(fr[0] == 0.0f && signbit(fr[0]), "vfnmsub signed zero");
    TEST_ASSERT(fr[1] == -FLT_MIN / 2.0f, "vfnmsub subnormal result");
    TEST_ASSERT(isnan(fr[2]), "vfnmsub Inf * 0 is NaN (float)");
    TEST_ASSERT(isnan(fr[3]), "vfnmsub NaN propagation (float)");
    TEST_ASSERT(isinf(fr[4]) && signbit(fr[4]), "vfnmsub float overflow is -Inf");
}

int main(void) {
    if (!check_fma()) { printf("FMA not supported\n"); return 1; }
    test_vfnmsub132ps();
    test_vfnmsub213ps();
    test_vfnmsub231ps();
    test_vfnmsub_pd();
    test_vfnmsub_sd();
    test_vfnmsub_ss();
    test_vfnmsub_special_and_fused();
    TEST_END();
}
