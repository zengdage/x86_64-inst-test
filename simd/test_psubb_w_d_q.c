/*
 * test_psubb_w_d_q.c - Test PSUBB/PSUBW/PSUBD/PSUBQ instructions
 *
 * PSUBB: Packed subtract bytes (wrapping).
 * PSUBW: Packed subtract words (wrapping).
 * PSUBD: Packed subtract doublewords (wrapping).
 * PSUBQ: Packed subtract quadwords (wrapping).
 *
 * Compile: gcc -o test_psubb_w_d_q simd/test_psubb_w_d_q.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_psubb_basic(void) {
    xmm_t a = { .u8 = {10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160} };
    xmm_t b = { .u8 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        uint8_t expected = (uint8_t)(a.u8[i] - b.u8[i]);
        TEST_ASSERT(dst.u8[i] == expected,
            "psubb [%d]: expected %u, got %u", i, expected, dst.u8[i]);
    }
}

static void test_psubb_underflow(void) {
    xmm_t a = { .u8 = {0, 0, 1, 128, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {1, 255, 2, 129, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 255, "psubb 0-1 wraps to 255: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 1, "psubb 0-255 wraps to 1: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 255, "psubb 1-2 wraps to 255: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 255, "psubb 128-129 wraps to 255: got %u", dst.u8[3]);
}

static void test_psubw_basic(void) {
    xmm_t a = { .u16 = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000} };
    xmm_t b = { .u16 = {100, 200, 300, 400, 500, 600, 700, 800} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 8; i++) {
        uint16_t expected = (uint16_t)(a.u16[i] - b.u16[i]);
        TEST_ASSERT(dst.u16[i] == expected,
            "psubw [%d]: expected %u, got %u", i, expected, dst.u16[i]);
    }
}

static void test_psubw_underflow(void) {
    xmm_t a = { .u16 = {0, 0, 0,0, 0,0,0,0} };
    xmm_t b = { .u16 = {1, 0xFFFF, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0xFFFF, "psubw 0-1 wraps: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 1, "psubw 0-0xFFFF wraps: got %u", dst.u16[1]);
}

static void test_psubd_basic(void) {
    xmm_t a = { .u32 = {100000, 200000, 300000, 400000} };
    xmm_t b = { .u32 = {1, 2, 3, 4} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.u32[i] == a.u32[i] - b.u32[i],
            "psubd [%d]: expected %u, got %u", i, a.u32[i] - b.u32[i], dst.u32[i]);
    }
}

static void test_psubd_underflow(void) {
    xmm_t a = { .u32 = {0, 0, 0, 0} };
    xmm_t b = { .u32 = {1, 0xFFFFFFFF, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "psubd 0-1 wraps: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 1, "psubd 0-0xFFFFFFFF wraps: got %u", dst.u32[1]);
}

static void test_psubq_basic(void) {
    xmm_t a = { .u64 = {1000000000ULL, 2000000000ULL} };
    xmm_t b = { .u64 = {1, 2} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 999999999ULL, "psubq [0]: got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 1999999998ULL, "psubq [1]: got %lu", dst.u64[1]);
}

static void test_psubq_underflow(void) {
    xmm_t a = { .u64 = {0, 0} };
    xmm_t b = { .u64 = {1, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL, "psubq 0-1 wraps: got 0x%016lx", dst.u64[0]);
}

int main(void) {
    TEST_START("PSUBB/PSUBW/PSUBD/PSUBQ instructions");
    test_psubb_basic();
    test_psubb_underflow();
    test_psubw_basic();
    test_psubw_underflow();
    test_psubd_basic();
    test_psubd_underflow();
    test_psubq_basic();
    test_psubq_underflow();
    TEST_END();
}
