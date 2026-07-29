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

int main(void) {
    TEST_START("VADDPS/VADDPD/VSUBPS/VSUBPD instructions (AVX 256-bit)");
    test_vaddps_256();
    test_vaddpd_256();
    test_vsubps_256();
    test_vsubpd_256();
    test_vaddps_three_operand();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
