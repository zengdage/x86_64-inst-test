/*
 * test_punpckh.c - Test PUNPCKHBW/PUNPCKHWD/PUNPCKHDQ/PUNPCKHQDQ instructions
 *
 * These interleave high elements from two operands.
 * PUNPCKHBW:  Interleave high bytes (bytes 8-15).
 * PUNPCKHWD:  Interleave high words (words 4-7).
 * PUNPCKHDQ:  Interleave high dwords (dwords 2-3).
 * PUNPCKHQDQ: Interleave high qwords (qword 1 from each).
 *
 * Compile: gcc -o test_punpckh simd/test_punpckh.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_punpckhbw(void) {
    xmm_t a = { .u8 = {0,0,0,0,0,0,0,0, 1,2,3,4,5,6,7,8} };
    xmm_t b = { .u8 = {0,0,0,0,0,0,0,0, 10,20,30,40,50,60,70,80} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpckhbw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 1 && dst.u8[1] == 10, "punpckhbw [0,1]: got %d,%d", dst.u8[0], dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 2 && dst.u8[3] == 20, "punpckhbw [2,3]: got %d,%d", dst.u8[2], dst.u8[3]);
    TEST_ASSERT(dst.u8[14] == 8 && dst.u8[15] == 80, "punpckhbw [14,15]: got %d,%d", dst.u8[14], dst.u8[15]);
}

static void test_punpckhwd(void) {
    xmm_t a = { .u16 = {0,0,0,0, 100, 200, 300, 400} };
    xmm_t b = { .u16 = {0,0,0,0, 1000, 2000, 3000, 4000} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpckhwd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 100 && dst.u16[1] == 1000, "punpckhwd [0,1]");
    TEST_ASSERT(dst.u16[2] == 200 && dst.u16[3] == 2000, "punpckhwd [2,3]");
    TEST_ASSERT(dst.u16[4] == 300 && dst.u16[5] == 3000, "punpckhwd [4,5]");
    TEST_ASSERT(dst.u16[6] == 400 && dst.u16[7] == 4000, "punpckhwd [6,7]");
}

static void test_punpckhdq(void) {
    xmm_t a = { .u32 = {0, 0, 10, 20} };
    xmm_t b = { .u32 = {0, 0, 30, 40} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpckhdq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 10, "punpckhdq [0]: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 30, "punpckhdq [1]: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 20, "punpckhdq [2]: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 40, "punpckhdq [3]: got %u", dst.u32[3]);
}

static void test_punpckhqdq(void) {
    xmm_t a = { .u64 = {0, 0xAAAABBBBCCCCDDDDULL} };
    xmm_t b = { .u64 = {0, 0x1111222233334444ULL} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpckhqdq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xAAAABBBBCCCCDDDDULL, "punpckhqdq [0]: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x1111222233334444ULL, "punpckhqdq [1]: got 0x%016lx", dst.u64[1]);
}

static void test_punpckhbw_boundary(void) {
    xmm_t a, b;
    for (int i = 0; i < 16; i++) { a.u8[i] = (uint8_t)(i); b.u8[i] = (uint8_t)(i + 100); }
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "punpckhbw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Result pairs high bytes: (a[8],b[8]), (a[9],b[9]), ... (a[15],b[15]) */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u8[i*2] == (uint8_t)(i + 8),
            "punpckhbw pair [%d] a: got %u", i, dst.u8[i*2]);
        TEST_ASSERT(dst.u8[i*2+1] == (uint8_t)(i + 108),
            "punpckhbw pair [%d] b: got %u", i, dst.u8[i*2+1]);
    }
}

int main(void) {
    TEST_START("PUNPCKHBW/PUNPCKHWD/PUNPCKHDQ/PUNPCKHQDQ instructions");
    test_punpckhbw();
    test_punpckhwd();
    test_punpckhdq();
    test_punpckhqdq();
    test_punpckhbw_boundary();
    TEST_END();
}
