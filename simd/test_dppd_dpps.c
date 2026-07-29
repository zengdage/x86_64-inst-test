/*
 * test_dppd_dpps.c - Test DPPD/DPPS instructions (SSE4.1)
 *
 * DPPS: Dot product of packed single-precision. imm8 controls which elements
 *       to multiply (bits 7:4) and where to store result (bits 3:0).
 * DPPD: Dot product of packed double-precision. imm8 bits 5:4 select multiply,
 *       bits 1:0 select store.
 *
 * Compile: gcc -o test_dppd_dpps simd/test_dppd_dpps.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>

static void test_dpps_all(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t dst;

    /* imm8 = 0xFF: multiply all 4, store to all 4 */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "dpps $0xFF, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 1*5 + 2*6 + 3*7 + 4*8 = 5+12+21+32 = 70 */
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == 70.0f,
            "dpps $FF [%d]: expected 70.0, got %f", i, dst.f32[i]);
    }
}

static void test_dpps_partial_multiply(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t dst;

    /* imm8 = 0x3F: multiply only elements 0,1 (bits 5:4 = 11), store to all */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "dpps $0x3F, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 1*5 + 2*6 = 17 */
    TEST_ASSERT(dst.f32[0] == 17.0f, "dpps $3F: expected 17.0, got %f", dst.f32[0]);
}

static void test_dpps_selective_store(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t dst;

    /* imm8 = 0xF1: multiply all, store to element 0 only */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "dpps $0xF1, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 70.0f, "dpps $F1 [0]: expected 70.0, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 0.0f, "dpps $F1 [1]: expected 0.0, got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 0.0f, "dpps $F1 [2]: expected 0.0, got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 0.0f, "dpps $F1 [3]: expected 0.0, got %f", dst.f32[3]);
}

static void test_dppd_basic(void) {
    xmm_t a = { .f64 = {3.0, 4.0} };
    xmm_t b = { .f64 = {5.0, 6.0} };
    xmm_t dst;

    /* imm8 = 0x33: multiply both, store to both */
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "dppd $0x33, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 3*5 + 4*6 = 15+24 = 39 */
    TEST_ASSERT(dst.f64[0] == 39.0, "dppd $33 [0]: expected 39.0, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 39.0, "dppd $33 [1]: expected 39.0, got %f", dst.f64[1]);
}

static void test_dppd_selective(void) {
    xmm_t a = { .f64 = {3.0, 4.0} };
    xmm_t b = { .f64 = {5.0, 6.0} };
    xmm_t dst;

    /* imm8 = 0x11: multiply element 0 only, store to element 0 only */
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "dppd $0x11, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 15.0, "dppd $11 [0]: expected 15.0, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 0.0, "dppd $11 [1]: expected 0.0, got %f", dst.f64[1]);
}

static void test_dpps_zero(void) {
    xmm_t a = { .f32 = {0.0f, 0.0f, 0.0f, 0.0f} };
    xmm_t b = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "dpps $0xFF, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 0.0f, "dpps with zeros: expected 0.0, got %f", dst.f32[0]);
}

int main(void) {
    TEST_START("DPPD/DPPS instructions (SSE4.1)");
    test_dpps_all();
    test_dpps_partial_multiply();
    test_dpps_selective_store();
    test_dppd_basic();
    test_dppd_selective();
    test_dpps_zero();
    TEST_END();
}
