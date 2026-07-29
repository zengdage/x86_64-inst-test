/*
 * test_vfmadd.c - Test VFMADD132PS/VFMADD213PS/VFMADD231PS/PD (FMA3)
 *
 * FMA3 fused multiply-add: computes (a*b)+c with a single rounding.
 * VFMADD132PS: dest = dest*src3 + src2  (PS = packed single)
 * VFMADD213PS: dest = src2*dest + src3
 * VFMADD231PS: dest = src2*src3 + dest
 * PD variants use packed double-precision.
 *
 * Compile: gcc -o test_vfmadd simd/test_vfmadd.c -O0 -mfma -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vfmadd132ps(void) {
    /* dest = dest * src3 + src2 */
    xmm_t a = { .f32 = {2.0f, 3.0f, 4.0f, 5.0f} };   /* dest */
    xmm_t b = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} }; /* src2 (addend) */
    xmm_t c = { .f32 = {3.0f, 4.0f, 5.0f, 6.0f} };    /* src3 (multiplier) */
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmadd132ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* dest = a*c + b: 2*3+10=16, 3*4+20=32, 4*5+30=50, 5*6+40=70 */
    TEST_ASSERT(dst.f32[0] == 16.0f, "vfmadd132ps [0]: expected 16, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 32.0f, "vfmadd132ps [1]: expected 32, got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 50.0f, "vfmadd132ps [2]: expected 50, got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 70.0f, "vfmadd132ps [3]: expected 70, got %f", dst.f32[3]);
}

static void test_vfmadd213ps(void) {
    /* dest = src2 * dest + src3 */
    xmm_t a = { .f32 = {2.0f, 3.0f, 4.0f, 5.0f} };   /* dest */
    xmm_t b = { .f32 = {3.0f, 4.0f, 5.0f, 6.0f} };   /* src2 (multiplier) */
    xmm_t c = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} }; /* src3 (addend) */
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmadd213ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* dest = b*a + c: 3*2+10=16, 4*3+20=32, 5*4+30=50, 6*5+40=70 */
    TEST_ASSERT(dst.f32[0] == 16.0f, "vfmadd213ps [0]: expected 16, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 32.0f, "vfmadd213ps [1]: expected 32, got %f", dst.f32[1]);
}

static void test_vfmadd231ps(void) {
    /* dest = src2 * src3 + dest */
    xmm_t a = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} }; /* dest (addend) */
    xmm_t b = { .f32 = {2.0f, 3.0f, 4.0f, 5.0f} };   /* src2 */
    xmm_t c = { .f32 = {3.0f, 4.0f, 5.0f, 6.0f} };   /* src3 */
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmadd231ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* dest = b*c + a: 2*3+10=16, 3*4+20=32, 4*5+30=50, 5*6+40=70 */
    TEST_ASSERT(dst.f32[0] == 16.0f, "vfmadd231ps [0]: expected 16, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[3] == 70.0f, "vfmadd231ps [3]: expected 70, got %f", dst.f32[3]);
}

static void test_vfmadd132pd(void) {
    xmm_t a = { .f64 = {2.0, 3.0} };
    xmm_t b = { .f64 = {10.0, 20.0} };
    xmm_t c = { .f64 = {3.0, 4.0} };
    xmm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%xmm0\n\t"
        "vmovapd %2, %%xmm1\n\t"
        "vfmadd132pd %3, %%xmm1, %%xmm0\n\t"
        "vmovapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.f64[0] == 16.0, "vfmadd132pd [0]: expected 16, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 32.0, "vfmadd132pd [1]: expected 32, got %f", dst.f64[1]);
}

static void test_vfmadd231ps_256(void) {
    ymm_t a = { .f32 = {1,1,1,1,1,1,1,1} };
    ymm_t b = { .f32 = {2,2,2,2,2,2,2,2} };
    ymm_t c = { .f32 = {3,3,3,3,3,3,3,3} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmovaps %2, %%ymm1\n\t"
        "vfmadd231ps %3, %%ymm1, %%ymm0\n\t"
        "vmovaps %%ymm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "ymm0", "ymm1"
    );
    /* dest = b*c + a = 2*3 + 1 = 7 */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == 7.0f,
            "vfmadd231ps 256 [%d]: expected 7, got %f", i, dst.f32[i]);
    }
}

static void test_vfmadd_zero(void) {
    xmm_t a = { .f32 = {0.0f, 0.0f, 0.0f, 0.0f} };
    xmm_t b = { .f32 = {5.0f, 5.0f, 5.0f, 5.0f} };
    xmm_t c = { .f32 = {3.0f, 3.0f, 3.0f, 3.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmadd132ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* 0*3 + 5 = 5 */
    TEST_ASSERT(dst.f32[0] == 5.0f, "vfmadd 0*x+y=y: got %f", dst.f32[0]);
}

int main(void) {
    TEST_START("VFMADD132PS/VFMADD213PS/VFMADD231PS/PD instructions (FMA3)");
    test_vfmadd132ps();
    test_vfmadd213ps();
    test_vfmadd231ps();
    test_vfmadd132pd();
    test_vfmadd231ps_256();
    test_vfmadd_zero();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
