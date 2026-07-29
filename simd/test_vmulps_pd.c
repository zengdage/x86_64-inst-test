/*
 * test_vmulps_pd.c - Test VMULPS/VMULPD/VDIVPS/VDIVPD (256-bit AVX)
 *
 * VEX-encoded 256-bit packed multiply/divide for single and double precision.
 *
 * Compile: gcc -o test_vmulps_pd simd/test_vmulps_pd.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

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

int main(void) {
    TEST_START("VMULPS/VMULPD/VDIVPS/VDIVPD instructions (AVX 256-bit)");
    test_vmulps_256();
    test_vmulpd_256();
    test_vdivps_256();
    test_vdivpd_256();
    test_vmulps_by_zero();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
