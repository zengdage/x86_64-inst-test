/*
 * test_vperm.c - Test VPERMD/VPERMQ/VPERMPD/VPERMPS instructions (AVX2)
 *
 * VPERMQ:  Permute 64-bit qwords across lanes using imm8 (AVX2).
 * VPERMPD: Permute 64-bit doubles across lanes using imm8 (AVX2).
 * VPERMD:  Permute 32-bit dwords across lanes using index vector (AVX2).
 * VPERMPS: Permute 32-bit floats across lanes using index vector (AVX2).
 *
 * Compile: gcc -o test_vperm simd/test_vperm.c -O0 -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vpermq_identity(void) {
    ymm_t src = { .i64 = {10, 20, 30, 40} };
    ymm_t dst;

    /* 0xE4 = 11_10_01_00 => identity */
    __asm__ volatile (
        "vmovdqa %1, %%ymm0\n\t"
        "vpermq $0xE4, %%ymm0, %%ymm1\n\t"
        "vmovdqa %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.i64[i] == src.i64[i],
            "vpermq identity [%d]: got %ld", i, dst.i64[i]);
    }
}

static void test_vpermq_reverse(void) {
    ymm_t src = { .i64 = {10, 20, 30, 40} };
    ymm_t dst;

    /* 0x1B = 00_01_10_11 => reverse */
    __asm__ volatile (
        "vmovdqa %1, %%ymm0\n\t"
        "vpermq $0x1B, %%ymm0, %%ymm1\n\t"
        "vmovdqa %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.i64[0] == 40, "vpermq reverse [0]: got %ld", dst.i64[0]);
    TEST_ASSERT(dst.i64[1] == 30, "vpermq reverse [1]: got %ld", dst.i64[1]);
    TEST_ASSERT(dst.i64[2] == 20, "vpermq reverse [2]: got %ld", dst.i64[2]);
    TEST_ASSERT(dst.i64[3] == 10, "vpermq reverse [3]: got %ld", dst.i64[3]);
}

static void test_vpermq_broadcast(void) {
    ymm_t src = { .i64 = {42, 0, 0, 0} };
    ymm_t dst;

    /* 0x00 => all select element 0 */
    __asm__ volatile (
        "vmovdqa %1, %%ymm0\n\t"
        "vpermq $0x00, %%ymm0, %%ymm1\n\t"
        "vmovdqa %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.i64[i] == 42, "vpermq broadcast [%d]: got %ld", i, dst.i64[i]);
    }
}

static void test_vpermpd(void) {
    ymm_t src = { .f64 = {1.0, 2.0, 3.0, 4.0} };
    ymm_t dst;

    /* 0x1B = reverse */
    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vpermpd $0x1B, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.f64[0] == 4.0, "vpermpd reverse [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 3.0, "vpermpd reverse [1]: got %f", dst.f64[1]);
    TEST_ASSERT(dst.f64[2] == 2.0, "vpermpd reverse [2]: got %f", dst.f64[2]);
    TEST_ASSERT(dst.f64[3] == 1.0, "vpermpd reverse [3]: got %f", dst.f64[3]);
}

static void test_vpermd(void) {
    ymm_t src = { .i32 = {10, 20, 30, 40, 50, 60, 70, 80} };
    ymm_t idx = { .i32 = {7, 6, 5, 4, 3, 2, 1, 0} };  /* reverse */
    ymm_t dst;

    __asm__ volatile (
        "vmovdqa %1, %%ymm0\n\t"
        "vmovdqa %2, %%ymm1\n\t"
        "vpermd %%ymm0, %%ymm1, %%ymm2\n\t"
        "vmovdqa %%ymm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "ymm0", "ymm1", "ymm2"
    );
    TEST_ASSERT(dst.i32[0] == 80, "vpermd reverse [0]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[7] == 10, "vpermd reverse [7]: got %d", dst.i32[7]);
}

static void test_vpermps(void) {
    ymm_t src = { .f32 = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f} };
    ymm_t idx = { .i32 = {0, 0, 0, 0, 0, 0, 0, 0} };  /* broadcast element 0 */
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmovdqa %2, %%ymm1\n\t"
        "vpermps %%ymm0, %%ymm1, %%ymm2\n\t"
        "vmovaps %%ymm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "ymm0", "ymm1", "ymm2"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == 1.0f, "vpermps broadcast [%d]: got %f", i, dst.f32[i]);
    }
}

int main(void) {
    TEST_START("VPERMD/VPERMQ/VPERMPD/VPERMPS instructions (AVX2)");
    test_vpermq_identity();
    test_vpermq_reverse();
    test_vpermq_broadcast();
    test_vpermpd();
    test_vpermd();
    test_vpermps();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
