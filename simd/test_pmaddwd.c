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

static int16_t pmaddubsw_reference(uint8_t a0, int8_t b0,
                                   uint8_t a1, int8_t b1) {
    int32_t sum = (int32_t)a0 * b0 + (int32_t)a1 * b1;
    if (sum > INT16_MAX) return INT16_MAX;
    if (sum < INT16_MIN) return INT16_MIN;
    return (int16_t)sum;
}

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
    TEST_ASSERT(dst.u32[1] == UINT32_C(0x80000000),
                "pmaddwd INT16_MIN pairs wrap only exceptional sum to INT32_MIN: %#x",
                dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0 && dst.u32[3] == 0,
                "pmaddwd zero pairs remain zero in all remaining lanes");
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
    xmm_t a = { .u8 = {
        255,191, 254,255, 255,128, 255,129,
        0,255, 255,255, 1,1, 255,255
    } };
    xmm_t b = { .i8 = {
        127,2, 127,2, -128,-1, -128,-1,
        127,127, 127,-128, 127,127, 0,0
    } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaddubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int lane = 0; lane < 8; lane++) {
        int16_t expected = pmaddubsw_reference(a.u8[2 * lane], b.i8[2 * lane],
                                               a.u8[2 * lane + 1], b.i8[2 * lane + 1]);
        TEST_ASSERT(dst.i16[lane] == expected,
                    "pmaddubsw saturation boundary lane %d: expected %d, got %d",
                    lane, expected, dst.i16[lane]);
    }
    TEST_ASSERT(dst.i16[0] == INT16_MAX && dst.i16[1] == INT16_MAX,
                "pmaddubsw covers exact positive maximum and positive overflow");
    TEST_ASSERT(dst.i16[2] == INT16_MIN && dst.i16[3] == INT16_MIN,
                "pmaddubsw covers exact negative minimum and negative overflow");
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
