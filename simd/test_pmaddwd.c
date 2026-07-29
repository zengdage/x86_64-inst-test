/*
 * test_pmaddwd.c - Test PMADDWD and PMADDUBSW instructions
 *
 * PMADDWD:  Multiply signed words pairwise, add adjacent pairs to produce dwords.
 *           dst[i] = src1[2i]*src2[2i] + src1[2i+1]*src2[2i+1]
 * PMADDUBSW: Multiply unsigned bytes by signed bytes, add adjacent pairs with
 *            signed saturation to produce words. (SSSE3)
 *
 * Compile: gcc -o test_pmaddwd simd/test_pmaddwd.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pmaddwd_basic(void) {
    xmm_t a = { .i16 = {1, 2, 3, 4, 5, 6, 7, 8} };
    xmm_t b = { .i16 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaddwd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* dst[0] = 1*10 + 2*20 = 50 */
    TEST_ASSERT(dst.i32[0] == 50, "pmaddwd [0]: expected 50, got %d", dst.i32[0]);
    /* dst[1] = 3*30 + 4*40 = 250 */
    TEST_ASSERT(dst.i32[1] == 250, "pmaddwd [1]: expected 250, got %d", dst.i32[1]);
    /* dst[2] = 5*50 + 6*60 = 610 */
    TEST_ASSERT(dst.i32[2] == 610, "pmaddwd [2]: expected 610, got %d", dst.i32[2]);
    /* dst[3] = 7*70 + 8*80 = 1130 */
    TEST_ASSERT(dst.i32[3] == 1130, "pmaddwd [3]: expected 1130, got %d", dst.i32[3]);
}

static void test_pmaddwd_negative(void) {
    xmm_t a = { .i16 = {-1, 1, 32767, -32768, 0,0,0,0} };
    xmm_t b = { .i16 = {1, -1, 1, 1, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaddwd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* -1*1 + 1*(-1) = -2 */
    TEST_ASSERT(dst.i32[0] == -2, "pmaddwd negative: expected -2, got %d", dst.i32[0]);
    /* 32767*1 + (-32768)*1 = -1 */
    TEST_ASSERT(dst.i32[1] == -1, "pmaddwd max+min: expected -1, got %d", dst.i32[1]);
}

static void test_pmaddwd_overflow(void) {
    /* 32767*32767 + 32767*32767 = 2*(32767^2) = 2*1073676289 = 2147352578 */
    xmm_t a = { .i16 = {32767, 32767, -32768, -32768, 0,0,0,0} };
    xmm_t b = { .i16 = {32767, 32767, -32768, -32768, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaddwd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    int32_t expected0 = 2 * (32767 * 32767);
    TEST_ASSERT(dst.i32[0] == expected0, "pmaddwd max*max: expected %d, got %d", expected0, dst.i32[0]);
    /* -32768*-32768 = 1073741824, *2 = 2147483648 which overflows to int32 */
    /* Result wraps in the 32-bit dest */
}

static void test_pmaddubsw_basic(void) {
    /* a is unsigned bytes, b is signed bytes */
    xmm_t a = { .u8 = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 1, 1, 0, 0, 0, 0} };
    xmm_t b = { .i8 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, -1, -1, 0, 0, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaddubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* dst[0] = 10*1 + 20*2 = 50 */
    TEST_ASSERT(dst.i16[0] == 50, "pmaddubsw [0]: expected 50, got %d", dst.i16[0]);
    /* dst[1] = 30*3 + 40*4 = 250 */
    TEST_ASSERT(dst.i16[1] == 250, "pmaddubsw [1]: expected 250, got %d", dst.i16[1]);
    /* dst[5] = 1*(-1) + 1*(-1) = -2 */
    TEST_ASSERT(dst.i16[5] == -2, "pmaddubsw [5]: expected -2, got %d", dst.i16[5]);
}

static void test_pmaddubsw_saturation(void) {
    xmm_t a = { .u8 = {255, 255, 255, 255, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {127, 127, -128, -128, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaddubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 255*127 + 255*127 = 64770, saturates to 32767 */
    TEST_ASSERT(dst.i16[0] == 32767, "pmaddubsw positive sat: expected 32767, got %d", dst.i16[0]);
    /* 255*(-128) + 255*(-128) = -65280, saturates to -32768 */
    TEST_ASSERT(dst.i16[1] == -32768, "pmaddubsw negative sat: expected -32768, got %d", dst.i16[1]);
}

int main(void) {
    TEST_START("PMADDWD/PMADDUBSW instructions");
    test_pmaddwd_basic();
    test_pmaddwd_negative();
    test_pmaddwd_overflow();
    test_pmaddubsw_basic();
    test_pmaddubsw_saturation();
    TEST_END();
}
