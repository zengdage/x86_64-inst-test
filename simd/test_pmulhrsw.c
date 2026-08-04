/*
 * test_pmulhrsw.c - Test PMULHRSW instruction (SSSE3)
 *
 * PMULHRSW: Packed multiply high with round and scale.
 * For each word pair: result = (((a * b) >> 14) + 1) >> 1
 * This is a fixed-point Q15 multiplication.
 *
 * Compile: gcc -o test_pmulhrsw simd/test_pmulhrsw.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static int16_t pmulhrsw_reference(int16_t a, int16_t b) {
    int32_t product = (int32_t)a * (int32_t)b;
    return (int16_t)((product + INT32_C(0x4000)) >> 15);
}

static void test_pmulhrsw_basic(void) {
    xmm_t a = { .i16 = {16384, -16384, 32767, -32768, 1, -1, 0, 100} };
    xmm_t b = { .i16 = {16384, 16384, 32767, 32767, 1, 1, 1, 100} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmulhrsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 16384 * 16384 = 268435456, >> 14 = 16384, +1 = 16385, >> 1 = 8192 */
    TEST_ASSERT(dst.i16[0] == 8192, "pmulhrsw 16384*16384: got %d", dst.i16[0]);
    /* -16384 * 16384 = -268435456, >> 14 = -16384, +1 = -16383, >> 1 = -8191 (arithmetic) */
    TEST_ASSERT(dst.i16[1] == -8192, "pmulhrsw -16384*16384: got %d", dst.i16[1]);
    /* 32767 * 32767 = 1073676289, >> 14 = 65534, +1 = 65535, >> 1 = 32767
     * But as signed 16-bit: intermediate truncation gives 32766 */
    TEST_ASSERT(dst.i16[2] == 32766, "pmulhrsw 32767*32767: got %d", dst.i16[2]);
    /* 1*1 = 1, >>14 = 0, +1 = 1, >>1 = 0 */
    TEST_ASSERT(dst.i16[4] == 0, "pmulhrsw 1*1: got %d", dst.i16[4]);
    /* 0*1 = 0 */
    TEST_ASSERT(dst.i16[6] == 0, "pmulhrsw 0*1: got %d", dst.i16[6]);
}

static void test_pmulhrsw_identity(void) {
    /* Q15 "1.0" is 32767, multiplying by it should approximately preserve the value */
    xmm_t a = { .i16 = {100, 1000, 10000, -100, -1000, -10000, 0, 1} };
    xmm_t b = { .i16 = {32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmulhrsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Should be approximately equal to a values */
    TEST_ASSERT(dst.i16[0] == 100, "pmulhrsw ~identity 100: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 1000, "pmulhrsw ~identity 1000: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 10000, "pmulhrsw ~identity 10000: got %d", dst.i16[2]);
}

static void test_pmulhrsw_zero(void) {
    xmm_t a = { .i16 = {0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i16 = {32767, -32768, 100, -100, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmulhrsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.i16[i] == 0, "pmulhrsw 0*x: got %d", dst.i16[i]);
    }
}

static void test_pmulhrsw_rounding_boundaries(void) {
    xmm_t a = { .i16 = {INT16_MIN, INT16_MAX, 1, -1, 0x4000, -0x4000, 0x2000, -0x2000} };
    xmm_t b = { .i16 = {INT16_MIN, INT16_MAX, INT16_MAX, INT16_MAX,
                         1, 1, 2, 2} };
    xmm_t dst;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t" "pmulhrsw %2, %%xmm0\n\t" "movdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0");
    for (int i = 0; i < 8; i++) {
        int16_t expected = pmulhrsw_reference(a.i16[i], b.i16[i]);
        TEST_ASSERT(dst.i16[i] == expected,
                    "pmulhrsw scalar reference lane %d: %d != %d", i, dst.i16[i], expected);
    }
    TEST_ASSERT(dst.u16[0] == UINT16_C(0x8000),
                "pmulhrsw INT16_MIN*INT16_MIN wraps to 0x8000");
}

int main(void) {
    TEST_START("PMULHRSW instruction (SSSE3)");
    test_pmulhrsw_basic();
    test_pmulhrsw_identity();
    test_pmulhrsw_zero();
    test_pmulhrsw_rounding_boundaries();
    TEST_END();
}
