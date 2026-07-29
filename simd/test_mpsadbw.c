/*
 * test_mpsadbw.c - Test MPSADBW instruction (SSE4.1)
 *
 * MPSADBW: Multiple packed sums of absolute differences of unsigned bytes.
 * Computes 8 SAD values between a 4-byte block from src2 and 8 consecutive
 * 4-byte blocks from src1. imm8 selects which blocks to use.
 *
 * Compile: gcc -o test_mpsadbw simd/test_mpsadbw.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_mpsadbw_basic(void) {
    xmm_t a = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t b = { .u8 = {0,1,2,3, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    /* imm8=0: src1 offset=0, src2 block=0 (bytes 0-3) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "mpsadbw $0, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* SAD between b[0:3]={0,1,2,3} and a[0:3]={0,1,2,3} = 0 */
    TEST_ASSERT(dst.u16[0] == 0, "mpsadbw [0] SAD(a[0:3],b[0:3])=0: got %u", dst.u16[0]);
    /* SAD between b[0:3]={0,1,2,3} and a[1:4]={1,2,3,4} = 4 */
    TEST_ASSERT(dst.u16[1] == 4, "mpsadbw [1] SAD(a[1:4],b[0:3])=4: got %u", dst.u16[1]);
    /* SAD between b[0:3] and a[2:5]={2,3,4,5} = 8 */
    TEST_ASSERT(dst.u16[2] == 8, "mpsadbw [2] SAD(a[2:5],b[0:3])=8: got %u", dst.u16[2]);
}

static void test_mpsadbw_identical(void) {
    xmm_t a = { .u8 = {42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42} };
    xmm_t b = { .u8 = {42,42,42,42, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "mpsadbw $0, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u16[i] == 0, "mpsadbw identical [%d]: expected 0, got %u", i, dst.u16[i]);
    }
}

static void test_mpsadbw_max_diff(void) {
    xmm_t a = { .u8 = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255} };
    xmm_t b = { .u8 = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "mpsadbw $0, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Each SAD = 4*255 = 1020 */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u16[i] == 1020, "mpsadbw max diff [%d]: expected 1020, got %u", i, dst.u16[i]);
    }
}

int main(void) {
    TEST_START("MPSADBW instruction (SSE4.1)");
    test_mpsadbw_basic();
    test_mpsadbw_identical();
    test_mpsadbw_max_diff();
    TEST_END();
}
