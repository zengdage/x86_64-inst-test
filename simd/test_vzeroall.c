/*
 * test_vzeroall.c - Test VZEROALL/VZEROUPPER instructions
 *
 * VZEROUPPER: Zero the upper 128 bits of all YMM registers (recommended
 *             before transitioning from AVX to SSE code).
 * VZEROALL:   Zero all YMM registers entirely (all 256 bits).
 *
 * Compile: gcc -o test_vzeroall simd/test_vzeroall.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vzeroupper(void) {
    ymm_t src = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vzeroupper\n\t"
        /* After vzeroupper, upper 128 bits of ymm0 are zeroed,
           but lower 128 bits (xmm0) are preserved */
        "vmovaps %%ymm0, %0"
        : "=m"(dst) : "m"(src) : "ymm0"
    );
    /* Lower 128 bits should be preserved */
    TEST_ASSERT(dst.f32[0] == 1.0f, "vzeroupper: xmm0[0] preserved, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 2.0f, "vzeroupper: xmm0[1] preserved, got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 3.0f, "vzeroupper: xmm0[2] preserved, got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 4.0f, "vzeroupper: xmm0[3] preserved, got %f", dst.f32[3]);
    /* Upper 128 bits should be zeroed */
    TEST_ASSERT(dst.f32[4] == 0.0f, "vzeroupper: upper[4] zeroed, got %f", dst.f32[4]);
    TEST_ASSERT(dst.f32[5] == 0.0f, "vzeroupper: upper[5] zeroed, got %f", dst.f32[5]);
    TEST_ASSERT(dst.f32[6] == 0.0f, "vzeroupper: upper[6] zeroed, got %f", dst.f32[6]);
    TEST_ASSERT(dst.f32[7] == 0.0f, "vzeroupper: upper[7] zeroed, got %f", dst.f32[7]);
}

static void test_vzeroall(void) {
    ymm_t src = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t dst0, dst1;

    __asm__ volatile (
        "vmovaps %2, %%ymm0\n\t"
        "vmovaps %2, %%ymm1\n\t"
        "vzeroall\n\t"
        "vmovaps %%ymm0, %0\n\t"
        "vmovaps %%ymm1, %1"
        : "=m"(dst0), "=m"(dst1) : "m"(src) : "ymm0", "ymm1"
    );
    /* All bits should be zeroed */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst0.f32[i] == 0.0f,
            "vzeroall: ymm0[%d] zeroed, got %f", i, dst0.f32[i]);
        TEST_ASSERT(dst1.f32[i] == 0.0f,
            "vzeroall: ymm1[%d] zeroed, got %f", i, dst1.f32[i]);
    }
}

static void test_vzeroupper_multiple_regs(void) {
    ymm_t src = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t dst0, dst1;

    __asm__ volatile (
        "vmovaps %2, %%ymm2\n\t"
        "vmovaps %2, %%ymm3\n\t"
        "vzeroupper\n\t"
        "vmovaps %%ymm2, %0\n\t"
        "vmovaps %%ymm3, %1"
        : "=m"(dst0), "=m"(dst1) : "m"(src) : "ymm2", "ymm3"
    );
    /* Lower preserved */
    TEST_ASSERT(dst0.f32[0] == 1.0f, "vzeroupper ymm2 low preserved");
    TEST_ASSERT(dst1.f32[0] == 1.0f, "vzeroupper ymm3 low preserved");
    /* Upper zeroed */
    TEST_ASSERT(dst0.f32[4] == 0.0f, "vzeroupper ymm2 upper zeroed");
    TEST_ASSERT(dst1.f32[4] == 0.0f, "vzeroupper ymm3 upper zeroed");
}

int main(void) {
    TEST_START("VZEROALL/VZEROUPPER instructions");
    test_vzeroupper();
    test_vzeroall();
    test_vzeroupper_multiple_regs();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
