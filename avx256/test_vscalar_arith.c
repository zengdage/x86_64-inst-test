/*
 * Test VADDSD/VADDSS/VSUBSD/VSUBSS/VMULSD/VMULSS/VDIVSD/VDIVSS
 * Scalar floating-point arithmetic (double and single precision)
 * Compile: gcc -o test_vscalar_arith avx256/test_vscalar_arith.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "../common.h"

static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}

static void test_vaddsd(void) {
    TEST_START("VADDSD");
    double a = 3.0, b = 4.0, r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vaddsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 7.0, "vaddsd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vaddsd %2, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 7.0, "vaddsd mem r=%f", r);
}

static void test_vaddss(void) {
    TEST_START("VADDSS");
    float a = 3.0f, b = 4.0f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vaddss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 7.0f, "vaddss r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vaddss %2, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 7.0f, "vaddss mem r=%f", r);
}

static void test_vsubsd(void) {
    TEST_START("VSUBSD");
    double a = 10.0, b = 3.0, r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vsubsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 7.0, "vsubsd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vsubsd %2, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 7.0, "vsubsd mem r=%f", r);
}

static void test_vsubss(void) {
    TEST_START("VSUBSS");
    float a = 10.0f, b = 3.0f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vsubss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 7.0f, "vsubss r=%f", r);
}

static void test_vmulsd(void) {
    TEST_START("VMULSD");
    double a = 3.0, b = 4.0, r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmulsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 12.0, "vmulsd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmulsd %2, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 12.0, "vmulsd mem r=%f", r);
}

static void test_vmulss(void) {
    TEST_START("VMULSS");
    float a = 3.0f, b = 4.0f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmulss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 12.0f, "vmulss r=%f", r);
}

static void test_vdivsd(void) {
    TEST_START("VDIVSD");
    double a = 12.0, b = 4.0, r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vdivsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 3.0, "vdivsd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vdivsd %2, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 3.0, "vdivsd mem r=%f", r);
    /* divide by zero -> inf */
    double zero = 0.0;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vdivsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(zero) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(isinf(r), "vdivsd div-by-zero -> inf");
}

static void test_vdivss(void) {
    TEST_START("VDIVSS");
    float a = 12.0f, b = 4.0f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vdivss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 3.0f, "vdivss r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vdivss %2, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 3.0f, "vdivss mem r=%f", r);
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vaddsd();
    test_vaddss();
    test_vsubsd();
    test_vsubss();
    test_vmulsd();
    test_vmulss();
    test_vdivsd();
    test_vdivss();
    TEST_END();
}
