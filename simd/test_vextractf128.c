/*
 * test_vextractf128.c - Test VEXTRACTF128/VEXTRACTI128 instructions (AVX)
 *
 * VEXTRACTF128: Extract 128-bit lane from 256-bit ymm to xmm/mem.
 *               imm8 bit 0: 0=low lane, 1=high lane.
 * VEXTRACTI128: Same but for integer data (AVX2).
 *
 * Compile: gcc -o test_vextractf128 simd/test_vextractf128.c -O0 -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vextractf128_low(void) {
    ymm_t src = { .f32 = {1.0f,2.0f,3.0f,4.0f, 5.0f,6.0f,7.0f,8.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vextractf128 $0, %%ymm0, %0"
        : "=m"(dst) : "m"(src) : "ymm0"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f && dst.f32[1] == 2.0f &&
                dst.f32[2] == 3.0f && dst.f32[3] == 4.0f,
        "vextractf128 low: expected {1,2,3,4}");
}

static void test_vextractf128_high(void) {
    ymm_t src = { .f32 = {1.0f,2.0f,3.0f,4.0f, 5.0f,6.0f,7.0f,8.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vextractf128 $1, %%ymm0, %0"
        : "=m"(dst) : "m"(src) : "ymm0"
    );
    TEST_ASSERT(dst.f32[0] == 5.0f && dst.f32[1] == 6.0f &&
                dst.f32[2] == 7.0f && dst.f32[3] == 8.0f,
        "vextractf128 high: expected {5,6,7,8}");
}

static void test_vextracti128_low(void) {
    ymm_t src = { .i32 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    __asm__ volatile (
        "vmovdqa %1, %%ymm0\n\t"
        "vextracti128 $0, %%ymm0, %0"
        : "=m"(dst) : "m"(src) : "ymm0"
    );
    TEST_ASSERT(dst.i32[0] == 10 && dst.i32[1] == 20 &&
                dst.i32[2] == 30 && dst.i32[3] == 40,
        "vextracti128 low: expected {10,20,30,40}");
}

static void test_vextracti128_high(void) {
    ymm_t src = { .i32 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    __asm__ volatile (
        "vmovdqa %1, %%ymm0\n\t"
        "vextracti128 $1, %%ymm0, %0"
        : "=m"(dst) : "m"(src) : "ymm0"
    );
    TEST_ASSERT(dst.i32[0] == 50 && dst.i32[1] == 60 &&
                dst.i32[2] == 70 && dst.i32[3] == 80,
        "vextracti128 high: expected {50,60,70,80}");
}

static void test_vextractf128_to_xmm(void) {
    ymm_t src = { .f64 = {1.1, 2.2, 3.3, 4.4} };
    xmm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vextractf128 $1, %%ymm0, %%xmm1\n\t"
        "vmovapd %%xmm1, %0"
        : "=m"(dst) : "m"(src) : "ymm0", "xmm1"
    );
    TEST_ASSERT(dst.f64[0] == 3.3, "vextractf128 to xmm [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 4.4, "vextractf128 to xmm [1]: got %f", dst.f64[1]);
}

int main(void) {
    TEST_START("VEXTRACTF128/VEXTRACTI128 instructions (AVX)");
    test_vextractf128_low();
    test_vextractf128_high();
    test_vextracti128_low();
    test_vextracti128_high();
    test_vextractf128_to_xmm();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
