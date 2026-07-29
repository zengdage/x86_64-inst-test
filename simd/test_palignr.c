/*
 * test_palignr.c - Test PALIGNR instruction (SSSE3)
 *
 * PALIGNR: Concatenate dest and src, shift right by imm8 bytes, take low 128 bits.
 * Conceptually: result = (dest:src >> (imm8*8))[127:0]
 *
 * Compile: gcc -o test_palignr simd/test_palignr.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_palignr_zero(void) {
    xmm_t a = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t b = { .u8 = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31} };
    xmm_t dst;

    /* shift 0: result = src (b) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "palignr $0, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == b.u8[i],
            "palignr $0 [%d]: expected %d, got %d", i, b.u8[i], dst.u8[i]);
    }
}

static void test_palignr_4(void) {
    xmm_t a = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t b = { .u8 = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31} };
    xmm_t dst;

    /* shift 4: result = (a:b >> 32)[127:0] = bytes 4..19 of concat(b,a) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "palignr $4, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* b[4..15], a[0..3] */
    for (int i = 0; i < 12; i++) {
        TEST_ASSERT(dst.u8[i] == (uint8_t)(20 + i),
            "palignr $4 [%d]: expected %d, got %d", i, 20 + i, dst.u8[i]);
    }
    for (int i = 12; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == (uint8_t)(i - 12),
            "palignr $4 [%d]: expected %d, got %d", i, i - 12, dst.u8[i]);
    }
}

static void test_palignr_16(void) {
    xmm_t a = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t b = { .u8 = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31} };
    xmm_t dst;

    /* shift 16: result = dest (a) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "palignr $16, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == a.u8[i],
            "palignr $16 [%d]: expected %d, got %d", i, a.u8[i], dst.u8[i]);
    }
}

static void test_palignr_8(void) {
    xmm_t a = { .u64 = {0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL} };
    xmm_t b = { .u64 = {0x1716151413121110ULL, 0x1F1E1D1C1B1A1918ULL} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "palignr $8, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* b[8..15], a[0..7] */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u8[i] == (uint8_t)(0x18 + i),
            "palignr $8 [%d]: expected 0x%02x, got 0x%02x", i, 0x18 + i, dst.u8[i]);
    }
    for (int i = 8; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == (uint8_t)(i - 8),
            "palignr $8 [%d]: expected 0x%02x, got 0x%02x", i, i - 8, dst.u8[i]);
    }
}

static void test_palignr_32(void) {
    xmm_t a = { .u64 = {0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL} };
    xmm_t b = { .u64 = {0x1716151413121110ULL, 0x1F1E1D1C1B1A1918ULL} };
    xmm_t dst;

    /* shift >= 32 => all zeros */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "palignr $32, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0,
        "palignr $32: all zeros");
}

int main(void) {
    TEST_START("PALIGNR instruction (SSSE3)");
    test_palignr_zero();
    test_palignr_4();
    test_palignr_16();
    test_palignr_8();
    test_palignr_32();
    TEST_END();
}
