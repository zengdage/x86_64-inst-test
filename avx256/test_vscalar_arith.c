/*
 * Test VADDSD/VADDSS/VSUBSD/VSUBSS/VMULSD/VMULSS/VDIVSD/VDIVSS
 * Scalar floating-point arithmetic (double and single precision)
 * Compile: gcc -o test_vscalar_arith avx256/test_vscalar_arith.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "../common.h"

#define RUN_SCALAR_SS(op, lhs, rhs, out) do { \
    __asm__ volatile("vmovaps %1, %%xmm0\n\t" "vmovaps %2, %%xmm1\n\t" \
        #op " %%xmm1, %%xmm0, %%xmm2\n\t" "vmovaps %%xmm2, %0" \
        : "=m"(out) : "m"(lhs), "m"(rhs) : "xmm0", "xmm1", "xmm2"); \
} while (0)
#define RUN_SCALAR_SD(op, lhs, rhs, out) do { \
    __asm__ volatile("vmovapd %1, %%xmm0\n\t" "vmovapd %2, %%xmm1\n\t" \
        #op " %%xmm1, %%xmm0, %%xmm2\n\t" "vmovapd %%xmm2, %0" \
        : "=m"(out) : "m"(lhs), "m"(rhs) : "xmm0", "xmm1", "xmm2"); \
} while (0)

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}
#else
#define check_avx() 1
#endif

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

static void test_scalar_special_values(void) {
    xmm_t a = { .f32 = {INFINITY, 11.0f, 22.0f, 33.0f} };
    xmm_t b = { .f32 = {-INFINITY, 1.0f, 2.0f, 3.0f} };
    xmm_t r;
    RUN_SCALAR_SS(vaddss, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f32[0]), "vaddss +Inf+-Inf is QNaN");
    TEST_ASSERT(r.f32[1] == 11.0f && r.f32[2] == 22.0f && r.f32[3] == 33.0f,
        "vaddss upper lanes copied from first source");
    b.f32[0] = INFINITY;
    RUN_SCALAR_SS(vsubss, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f32[0]), "vsubss Inf-Inf is QNaN");
    a.f32[0] = 0.0f; b.f32[0] = INFINITY;
    RUN_SCALAR_SS(vmulss, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f32[0]), "vmulss 0*Inf is QNaN");
    a.f32[0] = INFINITY; b.f32[0] = INFINITY;
    RUN_SCALAR_SS(vdivss, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f32[0]), "vdivss Inf/Inf is QNaN");
    a.f32[0] = FLT_MAX; b.f32[0] = 2.0f;
    RUN_SCALAR_SS(vmulss, a, b, r);
    TEST_ASSERT(isinf(r.f32[0]) && !signbit(r.f32[0]), "vmulss overflow");
    a.f32[0] = FLT_MIN; b.f32[0] = 0.5f;
    RUN_SCALAR_SS(vmulss, a, b, r);
    TEST_ASSERT(r.f32[0] == FLT_MIN / 2.0f, "vmulss subnormal");
    a.f32[0] = -0.0f; b.f32[0] = -0.0f;
    RUN_SCALAR_SS(vaddss, a, b, r);
    TEST_ASSERT(r.f32[0] == 0.0f && signbit(r.f32[0]), "vaddss negative zero");
    a.f32[0] = -1.0f; b.f32[0] = 0.0f;
    RUN_SCALAR_SS(vdivss, a, b, r);
    TEST_ASSERT(isinf(r.f32[0]) && signbit(r.f32[0]), "vdivss -1/+0 is -Inf");

    a.f64[0] = INFINITY; a.f64[1] = 123.0;
    b.f64[0] = -INFINITY; b.f64[1] = 456.0;
    RUN_SCALAR_SD(vaddsd, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f64[0]) && r.f64[1] == 123.0,
                "vaddsd invalid QNaN and upper-lane preservation");
    b.f64[0] = INFINITY;
    RUN_SCALAR_SD(vsubsd, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f64[0]), "vsubsd Inf-Inf is QNaN");
    a.f64[0] = 0.0; b.f64[0] = INFINITY;
    RUN_SCALAR_SD(vmulsd, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f64[0]), "vmulsd 0*Inf is QNaN");
    a.f64[0] = INFINITY; b.f64[0] = INFINITY;
    RUN_SCALAR_SD(vdivsd, a, b, r);
    TEST_ASSERT(IS_QNAN(r.f64[0]), "vdivsd Inf/Inf is QNaN");
    a.f64[0] = DBL_MAX; b.f64[0] = 2.0;
    RUN_SCALAR_SD(vmulsd, a, b, r);
    TEST_ASSERT(isinf(r.f64[0]), "vmulsd overflow");
    a.f64[0] = DBL_MIN; b.f64[0] = 0.5;
    RUN_SCALAR_SD(vmulsd, a, b, r);
    TEST_ASSERT(r.f64[0] == DBL_MIN / 2.0, "vmulsd subnormal");
    a.f64[0] = -0.0; b.f64[0] = -0.0;
    RUN_SCALAR_SD(vaddsd, a, b, r);
    TEST_ASSERT(r.f64[0] == 0.0 && signbit(r.f64[0]), "vaddsd negative zero");
    a.f64[0] = -1.0; b.f64[0] = 0.0;
    RUN_SCALAR_SD(vdivsd, a, b, r);
    TEST_ASSERT(isinf(r.f64[0]) && signbit(r.f64[0]), "vdivsd -1/+0 is -Inf");
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
    test_scalar_special_values();
    TEST_END();
}
