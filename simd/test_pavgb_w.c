/*
 * test_pavgb_w.c - Test PAVGB/PAVGW instructions
 *
 * PAVGB: Packed average of unsigned bytes with rounding: (a + b + 1) >> 1
 * PAVGW: Packed average of unsigned words with rounding: (a + b + 1) >> 1
 *
 * Compile: gcc -o test_pavgb_w simd/test_pavgb_w.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pavgb_basic(void) {
    xmm_t a = { .u8 = {0, 10, 100, 255, 200, 1, 0, 254, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {0, 20, 200, 255, 100, 0, 1, 0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pavgb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0, "pavgb (0+0+1)>>1=0: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 15, "pavgb (10+20+1)>>1=15: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 150, "pavgb (100+200+1)>>1=150: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 255, "pavgb (255+255+1)>>1=255: got %u", dst.u8[3]);
    TEST_ASSERT(dst.u8[4] == 150, "pavgb (200+100+1)>>1=150: got %u", dst.u8[4]);
    TEST_ASSERT(dst.u8[5] == 1, "pavgb (1+0+1)>>1=1: got %u", dst.u8[5]);
    TEST_ASSERT(dst.u8[6] == 1, "pavgb (0+1+1)>>1=1: got %u", dst.u8[6]);
    TEST_ASSERT(dst.u8[7] == 127, "pavgb (254+0+1)>>1=127: got %u", dst.u8[7]);
}

static void test_pavgw_basic(void) {
    xmm_t a = { .u16 = {0, 100, 1000, 65535, 0, 1, 0, 65534} };
    xmm_t b = { .u16 = {0, 200, 2000, 65535, 1, 0, 2, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pavgw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "pavgw (0+0+1)>>1=0: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 150, "pavgw (100+200+1)>>1=150: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 1500, "pavgw (1000+2000+1)>>1=1500: got %u", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 65535, "pavgw (65535+65535+1)>>1=65535: got %u", dst.u16[3]);
    TEST_ASSERT(dst.u16[4] == 1, "pavgw (0+1+1)>>1=1: got %u", dst.u16[4]);
    TEST_ASSERT(dst.u16[7] == 32767, "pavgw (65534+0+1)>>1=32767: got %u", dst.u16[7]);
}

static void test_pavgb_rounding(void) {
    /* Test that rounding is correct: (3+4+1)>>1 = 4, not 3 */
    xmm_t a = { .u8 = {3, 5, 7, 9, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {4, 6, 8, 10, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pavgb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 4, "pavgb (3+4+1)>>1=4: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 6, "pavgb (5+6+1)>>1=6: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 8, "pavgb (7+8+1)>>1=8: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 10, "pavgb (9+10+1)>>1=10: got %u", dst.u8[3]);
}

int main(void) {
    TEST_START("PAVGB/PAVGW instructions");
    test_pavgb_basic();
    test_pavgw_basic();
    test_pavgb_rounding();
    TEST_END();
}
