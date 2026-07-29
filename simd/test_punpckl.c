/*
 * test_punpckl.c - Test PUNPCKLBW/PUNPCKLWD/PUNPCKLDQ/PUNPCKLQDQ instructions
 *
 * These interleave low elements from two operands.
 * PUNPCKLBW:  Interleave low bytes.
 * PUNPCKLWD:  Interleave low words.
 * PUNPCKLDQ:  Interleave low dwords.
 * PUNPCKLQDQ: Interleave low qwords.
 *
 * Compile: gcc -o test_punpckl simd/test_punpckl.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_punpcklbw(void) {
    xmm_t a = { .u8 = {1,2,3,4,5,6,7,8, 0,0,0,0,0,0,0,0} };
    xmm_t b = { .u8 = {10,20,30,40,50,60,70,80, 0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpcklbw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Result: a[0],b[0],a[1],b[1],... */
    TEST_ASSERT(dst.u8[0] == 1 && dst.u8[1] == 10, "punpcklbw [0,1]: got %d,%d", dst.u8[0], dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 2 && dst.u8[3] == 20, "punpcklbw [2,3]: got %d,%d", dst.u8[2], dst.u8[3]);
    TEST_ASSERT(dst.u8[14] == 8 && dst.u8[15] == 80, "punpcklbw [14,15]: got %d,%d", dst.u8[14], dst.u8[15]);
}

static void test_punpcklwd(void) {
    xmm_t a = { .u16 = {100, 200, 300, 400, 0,0,0,0} };
    xmm_t b = { .u16 = {1000, 2000, 3000, 4000, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpcklwd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 100 && dst.u16[1] == 1000, "punpcklwd [0,1]");
    TEST_ASSERT(dst.u16[2] == 200 && dst.u16[3] == 2000, "punpcklwd [2,3]");
    TEST_ASSERT(dst.u16[4] == 300 && dst.u16[5] == 3000, "punpcklwd [4,5]");
    TEST_ASSERT(dst.u16[6] == 400 && dst.u16[7] == 4000, "punpcklwd [6,7]");
}

static void test_punpckldq(void) {
    xmm_t a = { .u32 = {10, 20, 0, 0} };
    xmm_t b = { .u32 = {30, 40, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpckldq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 10, "punpckldq [0]: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 30, "punpckldq [1]: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 20, "punpckldq [2]: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 40, "punpckldq [3]: got %u", dst.u32[3]);
}

static void test_punpcklqdq(void) {
    xmm_t a = { .u64 = {0xAAAABBBBCCCCDDDDULL, 0} };
    xmm_t b = { .u64 = {0x1111222233334444ULL, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpcklqdq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xAAAABBBBCCCCDDDDULL, "punpcklqdq [0]: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x1111222233334444ULL, "punpcklqdq [1]: got 0x%016lx", dst.u64[1]);
}

static void test_punpcklbw_with_zero(void) {
    xmm_t a = { .u8 = {0xFF, 0x80, 0x01, 0x00, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u64 = {0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpcklbw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Zero-extends each byte to word */
    TEST_ASSERT(dst.u16[0] == 0x00FF, "punpcklbw zero-extend 0xFF: got 0x%04x", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0x0080, "punpcklbw zero-extend 0x80: got 0x%04x", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0x0001, "punpcklbw zero-extend 0x01: got 0x%04x", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 0x0000, "punpcklbw zero-extend 0x00: got 0x%04x", dst.u16[3]);
}

int main(void) {
    TEST_START("PUNPCKLBW/PUNPCKLWD/PUNPCKLDQ/PUNPCKLQDQ instructions");
    test_punpcklbw();
    test_punpcklwd();
    test_punpckldq();
    test_punpcklqdq();
    test_punpcklbw_with_zero();
    TEST_END();
}
