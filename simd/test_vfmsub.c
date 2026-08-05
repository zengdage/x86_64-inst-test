/*
 * test_vfmsub.c - Test VFMSUB132PS/VFMSUB213PS/VFMSUB231PS/PD (FMA3)
 *
 * FMA3 fused multiply-subtract: computes (a*b)-c with a single rounding.
 * VFMSUB132PS: dest = dest*src3 - src2
 * VFMSUB213PS: dest = src2*dest - src3
 * VFMSUB231PS: dest = src2*src3 - dest
 *
 * Compile: gcc -o test_vfmsub simd/test_vfmsub.c -O0 -mfma -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <float.h>
#include <math.h>

static void test_vfmsub132ps(void) {
    /* dest = dest*src3 - src2 */
    xmm_t a = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t b = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t c = { .f32 = {3.0f, 4.0f, 5.0f, 6.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmsub132ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* dest = a*c - b: 5*3-1=14, 6*4-2=22, 7*5-3=32, 8*6-4=44 */
    TEST_ASSERT(dst.f32[0] == 14.0f, "vfmsub132ps [0]: expected 14, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 22.0f, "vfmsub132ps [1]: expected 22, got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 32.0f, "vfmsub132ps [2]: expected 32, got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 44.0f, "vfmsub132ps [3]: expected 44, got %f", dst.f32[3]);
}

static void test_vfmsub213ps(void) {
    /* dest = src2*dest - src3 */
    xmm_t a = { .f32 = {2.0f, 3.0f, 4.0f, 5.0f} };
    xmm_t b = { .f32 = {3.0f, 4.0f, 5.0f, 6.0f} };
    xmm_t c = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmsub213ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* dest = b*a - c: 3*2-1=5, 4*3-2=10, 5*4-3=17, 6*5-4=26 */
    TEST_ASSERT(dst.f32[0] == 5.0f, "vfmsub213ps [0]: expected 5, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 10.0f, "vfmsub213ps [1]: expected 10, got %f", dst.f32[1]);
}

static void test_vfmsub231ps(void) {
    /* dest = src2*src3 - dest */
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t c = { .f32 = {3.0f, 4.0f, 5.0f, 6.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmsub231ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* dest = b*c - a: 5*3-1=14, 6*4-2=22, 7*5-3=32, 8*6-4=44 */
    TEST_ASSERT(dst.f32[0] == 14.0f, "vfmsub231ps [0]: expected 14, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[3] == 44.0f, "vfmsub231ps [3]: expected 44, got %f", dst.f32[3]);
}

static void test_vfmsub132pd(void) {
    xmm_t a = { .f64 = {5.0, 6.0} };
    xmm_t b = { .f64 = {1.0, 2.0} };
    xmm_t c = { .f64 = {3.0, 4.0} };
    xmm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%xmm0\n\t"
        "vmovapd %2, %%xmm1\n\t"
        "vfmsub132pd %3, %%xmm1, %%xmm0\n\t"
        "vmovapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* 5*3-1=14, 6*4-2=22 */
    TEST_ASSERT(dst.f64[0] == 14.0, "vfmsub132pd [0]: expected 14, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 22.0, "vfmsub132pd [1]: expected 22, got %f", dst.f64[1]);
}

static void test_vfmsub_result_negative(void) {
    xmm_t a = { .f32 = {1.0f, 1.0f, 1.0f, 1.0f} };
    xmm_t b = { .f32 = {10.0f, 10.0f, 10.0f, 10.0f} };
    xmm_t c = { .f32 = {2.0f, 2.0f, 2.0f, 2.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmsub132ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1"
    );
    /* 1*2 - 10 = -8 */
    TEST_ASSERT(dst.f32[0] == -8.0f, "vfmsub negative result: got %f", dst.f32[0]);
}

static void test_vfmsub_special_and_fused(void) {
    xmm_t a = { .f64 = {0x1.0000000000001p+0, INFINITY} };
    xmm_t subtrahend = { .f64 = {1.0, 1.0} };
    xmm_t multiplier = { .f64 = {0x1.fffffffffffffp-1, 0.0} };
    xmm_t dst;
    __asm__ volatile (
        "vmovapd %1, %%xmm0\n\t"
        "vmovapd %2, %%xmm1\n\t"
        "vfmsub132pd %3, %%xmm1, %%xmm0\n\t"
        "vmovapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(subtrahend), "m"(multiplier) : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.f64[0] == 0x1.ffffffffffffep-54,
        "vfmsub fused single-round result: %a", dst.f64[0]);
    volatile double rounded_product = a.f64[0] * multiplier.f64[0];
    TEST_ASSERT(rounded_product - subtrahend.f64[0] == 0.0,
        "vfmsub discriminator requires ordinary multiply/subtract to cancel");
    TEST_ASSERT(IS_QNAN(dst.f64[1]), "vfmsub Inf * 0 is QNaN");

    a.f32[0] = NAN;      a.f32[1] = INFINITY; a.f32[2] = FLT_MAX; a.f32[3] = -0.0f;
    subtrahend.f32[0] = 1.0f; subtrahend.f32[1] = 1.0f;
    subtrahend.f32[2] = 0.0f; subtrahend.f32[3] = 0.0f;
    multiplier.f32[0] = 1.0f; multiplier.f32[1] = 0.0f;
    multiplier.f32[2] = 2.0f; multiplier.f32[3] = 2.0f;
    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vmovaps %2, %%xmm1\n\t"
        "vfmsub132ps %3, %%xmm1, %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(subtrahend), "m"(multiplier) : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(dst.f32[0]), "vfmsub QNaN propagation");
    TEST_ASSERT(IS_QNAN(dst.f32[1]), "vfmsub Inf * 0 is QNaN (float)");
    TEST_ASSERT(isinf(dst.f32[2]) && !signbit(dst.f32[2]), "vfmsub overflow is +Inf");
    TEST_ASSERT(dst.f32[3] == 0.0f && signbit(dst.f32[3]), "vfmsub preserves negative zero");
}

int main(void) {
    TEST_START("VFMSUB132PS/VFMSUB213PS/VFMSUB231PS/PD instructions (FMA3)");
    test_vfmsub132ps();
    test_vfmsub213ps();
    test_vfmsub231ps();
    test_vfmsub132pd();
    test_vfmsub_result_negative();
    test_vfmsub_special_and_fused();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
