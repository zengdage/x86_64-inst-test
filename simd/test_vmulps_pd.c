/*
 * test_vmulps_pd.c - Test VMULPS/VMULPD/VDIVPS/VDIVPD (256-bit AVX)
 *
 * VEX-encoded 256-bit packed multiply/divide for single and double precision.
 *
 * Compile: gcc -o test_vmulps_pd simd/test_vmulps_pd.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <float.h>
#include <math.h>

static void test_vmulps_256(void) {
    ymm_t a = { .f32 = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f} };
    ymm_t b = { .f32 = {2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f,9.0f} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmulps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        float expected = a.f32[i] * b.f32[i];
        TEST_ASSERT(dst.f32[i] == expected,
            "vmulps [%d]: expected %f, got %f", i, expected, dst.f32[i]);
    }
}

static void test_vmulpd_256(void) {
    ymm_t a = { .f64 = {1.5, 2.5, 3.5, 4.5} };
    ymm_t b = { .f64 = {2.0, 4.0, 6.0, 8.0} };
    ymm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vmulpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        double expected = a.f64[i] * b.f64[i];
        TEST_ASSERT(dst.f64[i] == expected,
            "vmulpd [%d]: expected %f, got %f", i, expected, dst.f64[i]);
    }
}

static void test_vdivps_256(void) {
    ymm_t a = { .f32 = {10.0f,20.0f,30.0f,40.0f,50.0f,60.0f,70.0f,80.0f} };
    ymm_t b = { .f32 = {2.0f,4.0f,5.0f,8.0f,10.0f,12.0f,14.0f,16.0f} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vdivps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        float expected = a.f32[i] / b.f32[i];
        TEST_ASSERT(dst.f32[i] == expected,
            "vdivps [%d]: expected %f, got %f", i, expected, dst.f32[i]);
    }
}

static void test_vdivpd_256(void) {
    ymm_t a = { .f64 = {100.0, 200.0, 300.0, 400.0} };
    ymm_t b = { .f64 = {10.0, 20.0, 30.0, 40.0} };
    ymm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vdivpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f64[i] == 10.0,
            "vdivpd [%d]: expected 10.0, got %f", i, dst.f64[i]);
    }
}

static void test_vmulps_by_zero(void) {
    ymm_t a = { .f32 = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f} };
    ymm_t b = { .f32 = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmulps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == 0.0f,
            "vmulps by zero [%d]: expected 0.0, got %f", i, dst.f32[i]);
    }
}

static void test_mul_div_special_values(void) {
    ymm_t a = { .f32 = {
        0.0f, -0.0f, INFINITY, FLT_MAX,
        FLT_MIN, NAN, -INFINITY, 0x1p-149f
    } };
    ymm_t b = { .f32 = {
        INFINITY, 2.0f, -2.0f, 2.0f,
        0.5f, 1.0f, -0.0f, 0.5f
    } };
    ymm_t dst;
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmulps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f32[0]), "vmulps 0 * Inf is NaN");
    TEST_ASSERT(dst.f32[1] == 0.0f && signbit(dst.f32[1]), "vmulps -0 * positive is -0");
    TEST_ASSERT(isinf(dst.f32[2]) && signbit(dst.f32[2]), "vmulps +Inf * negative is -Inf");
    TEST_ASSERT(isinf(dst.f32[3]) && !signbit(dst.f32[3]), "vmulps overflow to +Inf");
    TEST_ASSERT(dst.f32[4] == FLT_MIN / 2.0f, "vmulps subnormal result");
    TEST_ASSERT(isnan(dst.f32[5]), "vmulps NaN propagation");
    TEST_ASSERT(isnan(dst.f32[6]), "vmulps Inf * zero is NaN");
    TEST_ASSERT(dst.f32[7] == 0.0f, "vmulps minimum subnormal underflows to zero");

    a.f32[0] = INFINITY; a.f32[1] = 0.0f; a.f32[2] = 1.0f; a.f32[3] = -1.0f;
    a.f32[4] = 1.0f; a.f32[5] = -0.0f; a.f32[6] = FLT_MIN; a.f32[7] = NAN;
    b.f32[0] = INFINITY; b.f32[1] = 0.0f; b.f32[2] = 0.0f; b.f32[3] = 0.0f;
    b.f32[4] = INFINITY; b.f32[5] = 2.0f; b.f32[6] = 2.0f; b.f32[7] = 1.0f;
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vdivps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f32[0]), "vdivps Inf / Inf is NaN");
    TEST_ASSERT(isnan(dst.f32[1]), "vdivps 0 / 0 is NaN");
    TEST_ASSERT(isinf(dst.f32[2]) && !signbit(dst.f32[2]), "vdivps 1 / +0 is +Inf");
    TEST_ASSERT(isinf(dst.f32[3]) && signbit(dst.f32[3]), "vdivps -1 / +0 is -Inf");
    TEST_ASSERT(dst.f32[4] == 0.0f && !signbit(dst.f32[4]), "vdivps 1 / Inf is +0");
    TEST_ASSERT(dst.f32[5] == 0.0f && signbit(dst.f32[5]), "vdivps -0 / positive is -0");
    TEST_ASSERT(dst.f32[6] == FLT_MIN / 2.0f, "vdivps subnormal result");
    TEST_ASSERT(isnan(dst.f32[7]), "vdivps NaN propagation");

    a.f64[0] = 0.0; a.f64[1] = -0.0; a.f64[2] = INFINITY; a.f64[3] = DBL_MAX;
    b.f64[0] = INFINITY; b.f64[1] = 2.0; b.f64[2] = -2.0; b.f64[3] = 2.0;
    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vmulpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f64[0]), "vmulpd 0 * Inf is NaN");
    TEST_ASSERT(dst.f64[1] == 0.0 && signbit(dst.f64[1]), "vmulpd -0 * positive is -0");
    TEST_ASSERT(isinf(dst.f64[2]) && signbit(dst.f64[2]), "vmulpd +Inf * negative is -Inf");
    TEST_ASSERT(isinf(dst.f64[3]) && !signbit(dst.f64[3]), "vmulpd overflow to +Inf");

    a.f64[0] = INFINITY; a.f64[1] = 0.0; a.f64[2] = 1.0; a.f64[3] = DBL_MIN;
    b.f64[0] = INFINITY; b.f64[1] = 0.0; b.f64[2] = INFINITY; b.f64[3] = 2.0;
    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vdivpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f64[0]), "vdivpd Inf / Inf is NaN");
    TEST_ASSERT(isnan(dst.f64[1]), "vdivpd 0 / 0 is NaN");
    TEST_ASSERT(dst.f64[2] == 0.0 && !signbit(dst.f64[2]), "vdivpd 1 / Inf is +0");
    TEST_ASSERT(dst.f64[3] == DBL_MIN / 2.0, "vdivpd subnormal result");
}

int main(void) {
    TEST_START("VMULPS/VMULPD/VDIVPS/VDIVPD instructions (AVX 256-bit)");
    test_vmulps_256();
    test_vmulpd_256();
    test_vdivps_256();
    test_vdivpd_256();
    test_vmulps_by_zero();
    test_mul_div_special_values();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
