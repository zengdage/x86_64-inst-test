/*
 * test_vblend.c - Test VBLENDPS/VBLENDPD instructions (AVX)
 *
 * VBLENDPS: Blend packed single-precision using imm8 mask (256-bit).
 * VBLENDPD: Blend packed double-precision using imm8 mask (256-bit).
 * For each bit in mask: 0=select from first source, 1=select from second source.
 *
 * Compile: gcc -o test_vblend simd/test_vblend.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vblendps_none(void) {
    ymm_t a = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t b = { .f32 = {10,20,30,40,50,60,70,80} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vblendps $0x00, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == a.f32[i],
            "vblendps $0 [%d]: expected %f, got %f", i, a.f32[i], dst.f32[i]);
    }
}

static void test_vblendps_all(void) {
    ymm_t a = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t b = { .f32 = {10,20,30,40,50,60,70,80} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vblendps $0xFF, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == b.f32[i],
            "vblendps $FF [%d]: expected %f, got %f", i, b.f32[i], dst.f32[i]);
    }
}

static void test_vblendps_alternating(void) {
    ymm_t a = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t b = { .f32 = {10,20,30,40,50,60,70,80} };
    ymm_t dst;

    /* 0xAA = 10101010: elements 1,3,5,7 from b */
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vblendps $0xAA, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f, "vblendps $AA [0] from a: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 20.0f, "vblendps $AA [1] from b: got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[4] == 5.0f, "vblendps $AA [4] from a: got %f", dst.f32[4]);
    TEST_ASSERT(dst.f32[5] == 60.0f, "vblendps $AA [5] from b: got %f", dst.f32[5]);
}

static void test_vblendpd_basic(void) {
    ymm_t a = { .f64 = {1.0, 2.0, 3.0, 4.0} };
    ymm_t b = { .f64 = {10.0, 20.0, 30.0, 40.0} };
    ymm_t dst;

    /* 0x05 = 0101: elements 0,2 from b */
    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vblendpd $0x05, %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.f64[0] == 10.0, "vblendpd $5 [0] from b: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 2.0, "vblendpd $5 [1] from a: got %f", dst.f64[1]);
    TEST_ASSERT(dst.f64[2] == 30.0, "vblendpd $5 [2] from b: got %f", dst.f64[2]);
    TEST_ASSERT(dst.f64[3] == 4.0, "vblendpd $5 [3] from a: got %f", dst.f64[3]);
}

int main(void) {
    TEST_START("VBLENDPS/VBLENDPD instructions (AVX)");
    test_vblendps_none();
    test_vblendps_all();
    test_vblendps_alternating();
    test_vblendpd_basic();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
