/*
 * test_paddb_w_d_q.c - Test PADDB/PADDW/PADDD/PADDQ instructions
 *
 * PADDB: Packed add bytes (wrapping).
 * PADDW: Packed add words (wrapping).
 * PADDD: Packed add doublewords (wrapping).
 * PADDQ: Packed add quadwords (wrapping).
 *
 * Compile: gcc -o test_paddb_w_d_q simd/test_paddb_w_d_q.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_paddb_basic(void) {
    xmm_t a = { .u8 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    xmm_t b = { .u8 = {10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "paddb %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    for (int i = 0; i < 16; i++) {
        uint8_t expected = (uint8_t)(a.u8[i] + b.u8[i]);
        TEST_ASSERT(dst.u8[i] == expected,
            "paddb [%d]: expected %u, got %u", i, expected, dst.u8[i]);
    }
}

static void test_paddb_overflow(void) {
    xmm_t a = { .u8 = {255,128,200,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {1,128,100,0,   0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0, "paddb 255+1 wraps to 0: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 0, "paddb 128+128 wraps to 0: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 44, "paddb 200+100 wraps to 44: got %u", dst.u8[2]);
}

static void test_paddw_basic(void) {
    xmm_t a = { .u16 = {100, 200, 300, 400, 500, 600, 700, 800} };
    xmm_t b = { .u16 = {1, 2, 3, 4, 5, 6, 7, 8} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 8; i++) {
        uint16_t expected = (uint16_t)(a.u16[i] + b.u16[i]);
        TEST_ASSERT(dst.u16[i] == expected,
            "paddw [%d]: expected %u, got %u", i, expected, dst.u16[i]);
    }
}

static void test_paddw_overflow(void) {
    xmm_t a = { .u16 = {0xFFFF, 0x8000, 0,0, 0,0,0,0} };
    xmm_t b = { .u16 = {1, 0x8000, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "paddw 0xFFFF+1 wraps: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0, "paddw 0x8000+0x8000 wraps: got %u", dst.u16[1]);
}

static void test_paddd_basic(void) {
    xmm_t a = { .u32 = {100000, 200000, 300000, 400000} };
    xmm_t b = { .u32 = {1, 2, 3, 4} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        uint32_t expected = a.u32[i] + b.u32[i];
        TEST_ASSERT(dst.u32[i] == expected,
            "paddd [%d]: expected %u, got %u", i, expected, dst.u32[i]);
    }
}

static void test_paddd_overflow(void) {
    xmm_t a = { .u32 = {0xFFFFFFFF, 0x80000000, 0, 0} };
    xmm_t b = { .u32 = {1, 0x80000000, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0, "paddd 0xFFFFFFFF+1 wraps: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0, "paddd 0x80000000+0x80000000 wraps: got %u", dst.u32[1]);
}

static void test_paddq_basic(void) {
    xmm_t a = { .u64 = {1000000000ULL, 2000000000ULL} };
    xmm_t b = { .u64 = {1, 2} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 1000000001ULL, "paddq [0]: got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 2000000002ULL, "paddq [1]: got %lu", dst.u64[1]);
}

static void test_paddq_overflow(void) {
    xmm_t a = { .u64 = {0xFFFFFFFFFFFFFFFFULL, 0} };
    xmm_t b = { .u64 = {1, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0, "paddq max+1 wraps: got %lu", dst.u64[0]);
}

static void test_paddb_mem_operand(void) {
    xmm_t a = { .u8 = {10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160} };
    xmm_t b = { .u8 = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == (uint8_t)(a.u8[i] + 1),
            "paddb mem [%d]: expected %u, got %u", i, a.u8[i] + 1, dst.u8[i]);
    }
}

int main(void) {
    TEST_START("PADDB/PADDW/PADDD/PADDQ instructions");
    test_paddb_basic();
    test_paddb_overflow();
    test_paddw_basic();
    test_paddw_overflow();
    test_paddd_basic();
    test_paddd_overflow();
    test_paddq_basic();
    test_paddq_overflow();
    test_paddb_mem_operand();
    TEST_END();
}
