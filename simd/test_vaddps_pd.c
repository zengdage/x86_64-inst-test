/*
 * test_vaddps_pd.c - Test VADDPS/VADDPD/VSUBPS/VSUBPD (256-bit AVX)
 *
 * VEX-encoded 256-bit packed add/subtract for single and double precision.
 * Three-operand form: VADDPS ymm1, ymm2, ymm3/m256
 *
 * Compile: gcc -o test_vaddps_pd simd/test_vaddps_pd.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <float.h>
#include <math.h>

static void test_vaddps_256(void) {
    ymm_t a = { .f32 = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f} };
    ymm_t b = { .f32 = {10.0f,20.0f,30.0f,40.0f,50.0f,60.0f,70.0f,80.0f} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vaddps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        float expected = a.f32[i] + b.f32[i];
        TEST_ASSERT(dst.f32[i] == expected,
            "vaddps [%d]: expected %f, got %f", i, expected, dst.f32[i]);
    }
}

static void test_vaddpd_256(void) {
    ymm_t a = { .f64 = {1.5, 2.5, 3.5, 4.5} };
    ymm_t b = { .f64 = {10.0, 20.0, 30.0, 40.0} };
    ymm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vaddpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        double expected = a.f64[i] + b.f64[i];
        TEST_ASSERT(dst.f64[i] == expected,
            "vaddpd [%d]: expected %f, got %f", i, expected, dst.f64[i]);
    }
}

static void test_vsubps_256(void) {
    ymm_t a = { .f32 = {10.0f,20.0f,30.0f,40.0f,50.0f,60.0f,70.0f,80.0f} };
    ymm_t b = { .f32 = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vsubps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        float expected = a.f32[i] - b.f32[i];
        TEST_ASSERT(dst.f32[i] == expected,
            "vsubps [%d]: expected %f, got %f", i, expected, dst.f32[i]);
    }
}

static void test_vsubpd_256(void) {
    ymm_t a = { .f64 = {100.0, 200.0, 300.0, 400.0} };
    ymm_t b = { .f64 = {1.5, 2.5, 3.5, 4.5} };
    ymm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vsubpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        double expected = a.f64[i] - b.f64[i];
        TEST_ASSERT(dst.f64[i] == expected,
            "vsubpd [%d]: expected %f, got %f", i, expected, dst.f64[i]);
    }
}

static void test_vaddps_three_operand(void) {
    /* Verify three-operand form preserves source */
    ymm_t a = { .f32 = {1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f} };
    ymm_t b = { .f32 = {2.0f,2.0f,2.0f,2.0f,2.0f,2.0f,2.0f,2.0f} };
    ymm_t dst, src_after;

    __asm__ volatile (
        "vmovaps %2, %%ymm0\n\t"
        "vaddps %3, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0\n\t"
        "vmovaps %%ymm0, %1"
        : "=m"(dst), "=m"(src_after) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    /* ymm0 should still contain a */
    TEST_ASSERT(src_after.f32[0] == 1.0f, "vaddps 3-op: src preserved");
    TEST_ASSERT(dst.f32[0] == 3.0f, "vaddps 3-op: result correct");
}

static void test_add_sub_special_values(void) {
    ymm_t a = { .f32 = {
        INFINITY, -INFINITY, NAN, -0.0f,
        FLT_MAX, FLT_MIN / 2.0f, INFINITY, 0.0f
    } };
    ymm_t b = { .f32 = {
        -INFINITY, -INFINITY, 1.0f, -0.0f,
        FLT_MAX, FLT_MIN / 2.0f, INFINITY, -0.0f
    } };
    ymm_t dst;
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vaddps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f32[0]), "vaddps +Inf + -Inf is NaN");
    TEST_ASSERT(isinf(dst.f32[1]) && signbit(dst.f32[1]), "vaddps -Inf + -Inf is -Inf");
    TEST_ASSERT(isnan(dst.f32[2]), "vaddps NaN propagation");
    TEST_ASSERT(dst.f32[3] == 0.0f && signbit(dst.f32[3]), "vaddps -0 + -0 preserves -0");
    TEST_ASSERT(isinf(dst.f32[4]) && !signbit(dst.f32[4]), "vaddps overflow to +Inf");
    TEST_ASSERT(dst.f32[5] == FLT_MIN, "vaddps subnormal operands reach FLT_MIN");
    TEST_ASSERT(isinf(dst.f32[6]) && !signbit(dst.f32[6]), "vaddps +Inf + +Inf is +Inf");
    TEST_ASSERT(dst.f32[7] == 0.0f && !signbit(dst.f32[7]), "vaddps opposite signed zero gives +0");

    a.f32[0] = INFINITY;  b.f32[0] = INFINITY;
    a.f32[1] = -0.0f;     b.f32[1] = 0.0f;
    a.f32[2] = FLT_MAX;   b.f32[2] = -FLT_MAX;
    a.f32[3] = FLT_MIN;   b.f32[3] = FLT_MIN / 2.0f;
    a.f32[4] = NAN;       b.f32[4] = 1.0f;
    a.f32[5] = -INFINITY; b.f32[5] = INFINITY;
    a.f32[6] = 0.0f;      b.f32[6] = -0.0f;
    a.f32[7] = -0.0f;     b.f32[7] = -0.0f;
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vsubps %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f32[0]), "vsubps Inf - Inf is NaN");
    TEST_ASSERT(dst.f32[1] == 0.0f && signbit(dst.f32[1]), "vsubps -0 - +0 is -0");
    TEST_ASSERT(isinf(dst.f32[2]) && !signbit(dst.f32[2]), "vsubps overflow to +Inf");
    TEST_ASSERT(dst.f32[3] == FLT_MIN / 2.0f, "vsubps subnormal result");
    TEST_ASSERT(isnan(dst.f32[4]), "vsubps NaN propagation");
    TEST_ASSERT(isinf(dst.f32[5]) && signbit(dst.f32[5]), "vsubps -Inf - +Inf is -Inf");
    TEST_ASSERT(dst.f32[6] == 0.0f && !signbit(dst.f32[6]), "vsubps +0 - -0 is +0");
    TEST_ASSERT(dst.f32[7] == 0.0f && !signbit(dst.f32[7]), "vsubps -0 - -0 is +0");

    a.f64[0] = INFINITY; a.f64[1] = -0.0; a.f64[2] = DBL_MAX; a.f64[3] = DBL_MIN / 2.0;
    b.f64[0] = -INFINITY; b.f64[1] = -0.0; b.f64[2] = DBL_MAX; b.f64[3] = DBL_MIN / 2.0;
    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vaddpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f64[0]), "vaddpd +Inf + -Inf is NaN");
    TEST_ASSERT(dst.f64[1] == 0.0 && signbit(dst.f64[1]), "vaddpd -0 + -0 preserves -0");
    TEST_ASSERT(isinf(dst.f64[2]) && !signbit(dst.f64[2]), "vaddpd overflow to +Inf");
    TEST_ASSERT(dst.f64[3] == DBL_MIN, "vaddpd subnormal operands reach DBL_MIN");

    a.f64[0] = INFINITY; a.f64[1] = -0.0; a.f64[2] = DBL_MAX; a.f64[3] = DBL_MIN;
    b.f64[0] = INFINITY; b.f64[1] = 0.0; b.f64[2] = -DBL_MAX; b.f64[3] = DBL_MIN / 2.0;
    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vsubpd %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(isnan(dst.f64[0]), "vsubpd Inf - Inf is NaN");
    TEST_ASSERT(dst.f64[1] == 0.0 && signbit(dst.f64[1]), "vsubpd -0 - +0 is -0");
    TEST_ASSERT(isinf(dst.f64[2]) && !signbit(dst.f64[2]), "vsubpd overflow to +Inf");
    TEST_ASSERT(dst.f64[3] == DBL_MIN / 2.0, "vsubpd subnormal result");
}

int main(void) {
    TEST_START("VADDPS/VADDPD/VSUBPS/VSUBPD instructions (AVX 256-bit)");
    test_vaddps_256();
    test_vaddpd_256();
    test_vsubps_256();
    test_vsubpd_256();
    test_vaddps_three_operand();
    test_add_sub_special_values();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
