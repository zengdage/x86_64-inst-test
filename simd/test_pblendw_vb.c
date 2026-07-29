/*
 * test_pblendw_vb.c - Test PBLENDW and PBLENDVB instructions
 *
 * PBLENDW:  Blend words from two xmm registers based on imm8 mask (SSE4.1).
 *           For each bit in imm8: 0=select from dest, 1=select from src.
 * PBLENDVB: Variable blend bytes using xmm0 as implicit mask (SSE4.1).
 *           For each byte: high bit of mask byte selects src (1) or dest (0).
 *
 * Compile: gcc -o test_pblendw_vb simd/test_pblendw_vb.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pblendw_none(void) {
    xmm_t a = { .u16 = {1,2,3,4,5,6,7,8} };
    xmm_t b = { .u16 = {10,20,30,40,50,60,70,80} };
    xmm_t dst;

    /* imm8=0: all from dest (a) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pblendw $0x00, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u16[i] == a.u16[i],
            "pblendw $0 [%d]: expected %u, got %u", i, a.u16[i], dst.u16[i]);
    }
}

static void test_pblendw_all(void) {
    xmm_t a = { .u16 = {1,2,3,4,5,6,7,8} };
    xmm_t b = { .u16 = {10,20,30,40,50,60,70,80} };
    xmm_t dst;

    /* imm8=0xFF: all from src (b) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pblendw $0xFF, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u16[i] == b.u16[i],
            "pblendw $FF [%d]: expected %u, got %u", i, b.u16[i], dst.u16[i]);
    }
}

static void test_pblendw_mixed(void) {
    xmm_t a = { .u16 = {1,2,3,4,5,6,7,8} };
    xmm_t b = { .u16 = {10,20,30,40,50,60,70,80} };
    xmm_t dst;

    /* imm8=0xAA = 10101010: words 1,3,5,7 from src */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pblendw $0xAA, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 1, "pblendw $AA [0] from dest: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 20, "pblendw $AA [1] from src: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 3, "pblendw $AA [2] from dest: got %u", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 40, "pblendw $AA [3] from src: got %u", dst.u16[3]);
}

static void test_pblendvb_all_from_src(void) {
    xmm_t a = { .u8 = {1,2,3,4,5,6,7,8, 9,10,11,12,13,14,15,16} };
    xmm_t b = { .u8 = {101,102,103,104,105,106,107,108, 109,110,111,112,113,114,115,116} };
    xmm_t mask = { .u8 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                           0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %3, %%xmm0\n\t"   /* mask in xmm0 */
        "movdqa %1, %%xmm1\n\t"
        "pblendvb %%xmm0, %2, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(mask) : "xmm0", "xmm1"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == b.u8[i],
            "pblendvb all src [%d]: expected %u, got %u", i, b.u8[i], dst.u8[i]);
    }
}

static void test_pblendvb_mixed(void) {
    xmm_t a = { .u8 = {1,2,3,4, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {10,20,30,40, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t mask = { .u8 = {0x80, 0x00, 0x80, 0x00, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %3, %%xmm0\n\t"
        "movdqa %1, %%xmm1\n\t"
        "pblendvb %%xmm0, %2, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(mask) : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.u8[0] == 10, "pblendvb mixed [0] from src: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 2, "pblendvb mixed [1] from dest: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 30, "pblendvb mixed [2] from src: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 4, "pblendvb mixed [3] from dest: got %u", dst.u8[3]);
}

int main(void) {
    TEST_START("PBLENDW/PBLENDVB instructions");
    test_pblendw_none();
    test_pblendw_all();
    test_pblendw_mixed();
    test_pblendvb_all_from_src();
    test_pblendvb_mixed();
    TEST_END();
}
