/*
 * test_unpcklps_pd.c - Test UNPCKLPS/UNPCKLPD/UNPCKHPS/UNPCKHPD instructions
 *
 * UNPCKLPS: Interleave low single-precision floats.
 * UNPCKLPD: Interleave low double-precision floats.
 * UNPCKHPS: Interleave high single-precision floats.
 * UNPCKHPD: Interleave high double-precision floats.
 *
 * Compile: gcc -o test_unpcklps_pd simd/test_unpcklps_pd.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_unpcklps(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "unpcklps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f, "unpcklps [0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 5.0f, "unpcklps [1]: got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 2.0f, "unpcklps [2]: got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 6.0f, "unpcklps [3]: got %f", dst.f32[3]);
}

static void test_unpckhps(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "unpckhps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 3.0f, "unpckhps [0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 7.0f, "unpckhps [1]: got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 4.0f, "unpckhps [2]: got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 8.0f, "unpckhps [3]: got %f", dst.f32[3]);
}

static void test_unpcklpd(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {3.0, 4.0} };
    xmm_t dst;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "unpcklpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 1.0, "unpcklpd [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 3.0, "unpcklpd [1]: got %f", dst.f64[1]);
}

static void test_unpckhpd(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {3.0, 4.0} };
    xmm_t dst;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "unpckhpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 2.0, "unpckhpd [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 4.0, "unpckhpd [1]: got %f", dst.f64[1]);
}

static void test_unpcklps_mem(void) {
    xmm_t a = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} };
    xmm_t b = { .f32 = {50.0f, 60.0f, 70.0f, 80.0f} };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "unpcklps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 10.0f && dst.f32[1] == 50.0f &&
                dst.f32[2] == 20.0f && dst.f32[3] == 60.0f,
        "unpcklps mem operand");
}

int main(void) {
    TEST_START("UNPCKLPS/UNPCKLPD/UNPCKHPS/UNPCKHPD instructions");
    test_unpcklps();
    test_unpckhps();
    test_unpcklpd();
    test_unpckhpd();
    test_unpcklps_mem();
    TEST_END();
}
