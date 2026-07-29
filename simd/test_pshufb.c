/*
 * test_pshufb.c - Test PSHUFB instruction (SSSE3)
 *
 * PSHUFB: Packed shuffle bytes. Each byte in the destination is selected from
 * the source by the corresponding index byte in the mask. If the high bit of
 * the mask byte is set, the result byte is zeroed.
 *
 * Compile: gcc -o test_pshufb simd/test_pshufb.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pshufb_identity(void) {
    xmm_t src = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t mask = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pshufb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src), "m"(mask) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == (uint8_t)i,
            "pshufb identity [%d]: expected %d, got %d", i, i, dst.u8[i]);
    }
}

static void test_pshufb_reverse(void) {
    xmm_t src = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t mask = { .u8 = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pshufb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src), "m"(mask) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == (uint8_t)(15 - i),
            "pshufb reverse [%d]: expected %d, got %d", i, 15 - i, dst.u8[i]);
    }
}

static void test_pshufb_broadcast(void) {
    xmm_t src = { .u8 = {42,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} };
    xmm_t mask = { .u8 = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pshufb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src), "m"(mask) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == 42,
            "pshufb broadcast [%d]: expected 42, got %d", i, dst.u8[i]);
    }
}

static void test_pshufb_zero_mask(void) {
    xmm_t src = { .u8 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    /* High bit set => zero output */
    xmm_t mask = { .u8 = {0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
                           0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pshufb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src), "m"(mask) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == 0,
            "pshufb zero mask [%d]: expected 0, got %d", i, dst.u8[i]);
    }
}

static void test_pshufb_mixed(void) {
    xmm_t src = { .u8 = {10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160} };
    xmm_t mask = { .u8 = {0, 0x80, 3, 0x80, 7, 15, 0, 1, 0x80, 0x80, 5, 10, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pshufb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src), "m"(mask) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 10, "pshufb mixed [0]: expected 10, got %d", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 0, "pshufb mixed [1]: expected 0 (zeroed), got %d", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 40, "pshufb mixed [2]: expected 40, got %d", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 0, "pshufb mixed [3]: expected 0 (zeroed), got %d", dst.u8[3]);
    TEST_ASSERT(dst.u8[4] == 80, "pshufb mixed [4]: expected 80, got %d", dst.u8[4]);
    TEST_ASSERT(dst.u8[5] == 160, "pshufb mixed [5]: expected 160, got %d", dst.u8[5]);
}

static void test_pshufb_only_low_nibble(void) {
    /* Index uses only low 4 bits (bits 3:0), bit 7 is the zero flag */
    xmm_t src = { .u8 = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x11,0x22,
                          0x33,0x44,0x55,0x66,0x77,0x88,0x99,0x00} };
    /* index 0x13 = bit7=0, low4=3 => picks src[3] */
    xmm_t mask = { .u8 = {0x13, 0x70, 0,0,0,0,0,0, 0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pshufb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src), "m"(mask) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0xDD, "pshufb index 0x13 -> src[3]: got 0x%02x", dst.u8[0]);
    /* 0x70: bit7=0, low4=0 => src[0] */
    TEST_ASSERT(dst.u8[1] == 0xAA, "pshufb index 0x70 -> src[0]: got 0x%02x", dst.u8[1]);
}

int main(void) {
    TEST_START("PSHUFB instruction (SSSE3)");
    test_pshufb_identity();
    test_pshufb_reverse();
    test_pshufb_broadcast();
    test_pshufb_zero_mask();
    test_pshufb_mixed();
    test_pshufb_only_low_nibble();
    TEST_END();
}
