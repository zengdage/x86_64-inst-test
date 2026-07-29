/*
 * test_pabs.c - Test PABSB/PABSW/PABSD instructions (SSSE3)
 *
 * PABSB: Packed absolute value of bytes.
 * PABSW: Packed absolute value of words.
 * PABSD: Packed absolute value of dwords.
 * Note: abs(MIN_VALUE) wraps (e.g., abs(-128) = -128 for bytes).
 *
 * Compile: gcc -o test_pabs simd/test_pabs.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pabsb(void) {
    xmm_t src = { .i8 = {0, 1, -1, 127, -127, -128, 50, -50, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pabsb %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0, "pabsb |0|=0: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 1, "pabsb |1|=1: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 1, "pabsb |-1|=1: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 127, "pabsb |127|=127: got %u", dst.u8[3]);
    TEST_ASSERT(dst.u8[4] == 127, "pabsb |-127|=127: got %u", dst.u8[4]);
    /* -128 wraps to -128 (0x80) because abs(-128) can't be represented */
    TEST_ASSERT(dst.u8[5] == 128, "pabsb |-128|=128 (0x80): got %u", dst.u8[5]);
    TEST_ASSERT(dst.u8[6] == 50, "pabsb |50|=50: got %u", dst.u8[6]);
    TEST_ASSERT(dst.u8[7] == 50, "pabsb |-50|=50: got %u", dst.u8[7]);
}

static void test_pabsw(void) {
    xmm_t src = { .i16 = {0, 1, -1, 32767, -32767, -32768, 1000, -1000} };
    xmm_t dst;

    __asm__ volatile (
        "pabsw %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "pabsw |0|=0: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 1, "pabsw |1|=1: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 1, "pabsw |-1|=1: got %u", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 32767, "pabsw |32767|: got %u", dst.u16[3]);
    TEST_ASSERT(dst.u16[4] == 32767, "pabsw |-32767|: got %u", dst.u16[4]);
    TEST_ASSERT(dst.u16[5] == 32768, "pabsw |-32768|=32768 (wraps): got %u", dst.u16[5]);
    TEST_ASSERT(dst.u16[6] == 1000, "pabsw |1000|: got %u", dst.u16[6]);
    TEST_ASSERT(dst.u16[7] == 1000, "pabsw |-1000|: got %u", dst.u16[7]);
}

static void test_pabsd(void) {
    xmm_t src = { .i32 = {0, -1, 0x7FFFFFFF, (int32_t)0x80000000} };
    xmm_t dst;

    __asm__ volatile (
        "pabsd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0, "pabsd |0|=0: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 1, "pabsd |-1|=1: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0x7FFFFFFF, "pabsd |INT_MAX|: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0x80000000, "pabsd |INT_MIN| wraps: got 0x%08x", dst.u32[3]);
}

static void test_pabsd_mem(void) {
    xmm_t src = { .i32 = {-42, 42, -100000, 100000} };
    xmm_t dst;

    __asm__ volatile (
        "pabsd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 42, "pabsd |-42|: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 42, "pabsd |42|: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 100000, "pabsd |-100000|: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 100000, "pabsd |100000|: got %u", dst.u32[3]);
}

int main(void) {
    TEST_START("PABSB/PABSW/PABSD instructions (SSSE3)");
    test_pabsb();
    test_pabsw();
    test_pabsd();
    test_pabsd_mem();
    TEST_END();
}
