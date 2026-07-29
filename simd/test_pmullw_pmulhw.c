/*
 * test_pmullw_pmulhw.c - Test PMULLW/PMULHW/PMULHUW/PMULDQ/PMULUDQ/PMULLD instructions
 *
 * PMULLW:  Packed multiply words, store low 16 bits of each product.
 * PMULHW:  Packed multiply signed words, store high 16 bits.
 * PMULHUW: Packed multiply unsigned words, store high 16 bits.
 * PMULDQ:  Packed multiply signed dwords (even elements), produce 64-bit results.
 * PMULUDQ: Packed multiply unsigned dwords (even elements), produce 64-bit results.
 * PMULLD:  Packed multiply signed dwords, store low 32 bits of each product.
 *
 * Compile: gcc -o test_pmullw_pmulhw simd/test_pmullw_pmulhw.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pmullw_basic(void) {
    xmm_t a = { .i16 = {10, -10, 100, -100, 1, -1, 0, 32767} };
    xmm_t b = { .i16 = {20, 20, 200, 200, 1, 1, 1, 2} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmullw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == (int16_t)(10*20), "pmullw 10*20: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == (int16_t)(-10*20), "pmullw -10*20: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[4] == 1, "pmullw 1*1: got %d", dst.i16[4]);
    TEST_ASSERT(dst.i16[5] == (int16_t)(-1), "pmullw -1*1: got %d", dst.i16[5]);
    TEST_ASSERT(dst.i16[6] == 0, "pmullw 0*1: got %d", dst.i16[6]);
}

static void test_pmullw_overflow(void) {
    xmm_t a = { .i16 = {32767, -32768, 0,0, 0,0,0,0} };
    xmm_t b = { .i16 = {2, 2, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmullw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 32767*2 = 65534, low 16 bits = 0xFFFE = -2 as signed */
    TEST_ASSERT(dst.i16[0] == (int16_t)0xFFFE, "pmullw 32767*2 low: got %d", dst.i16[0]);
}

static void test_pmulhw_basic(void) {
    xmm_t a = { .i16 = {32767, -32768, 100, -1, 0,0,0,0} };
    xmm_t b = { .i16 = {2, 2, 100, -1, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmulhw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 32767*2 = 65534, high 16 bits = 0 */
    TEST_ASSERT(dst.i16[0] == 0, "pmulhw 32767*2 high: got %d", dst.i16[0]);
    /* -32768*2 = -65536, high = -1 */
    TEST_ASSERT(dst.i16[1] == -1, "pmulhw -32768*2 high: got %d", dst.i16[1]);
    /* 100*100 = 10000, high = 0 */
    TEST_ASSERT(dst.i16[2] == 0, "pmulhw 100*100 high: got %d", dst.i16[2]);
    /* (-1)*(-1) = 1, high = 0 */
    TEST_ASSERT(dst.i16[3] == 0, "pmulhw -1*-1 high: got %d", dst.i16[3]);
}

static void test_pmulhuw_basic(void) {
    xmm_t a = { .u16 = {65535, 256, 1, 0, 0,0,0,0} };
    xmm_t b = { .u16 = {65535, 256, 1, 0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmulhuw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 65535*65535 = 4294836225, high 16 = 65534 */
    TEST_ASSERT(dst.u16[0] == 65534, "pmulhuw 65535*65535 high: got %u", dst.u16[0]);
    /* 256*256 = 65536, high 16 = 1 */
    TEST_ASSERT(dst.u16[1] == 1, "pmulhuw 256*256 high: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0, "pmulhuw 1*1 high: got %u", dst.u16[2]);
}

static void test_pmuludq_basic(void) {
    xmm_t a = { .u32 = {0xFFFFFFFF, 0, 100, 0} };
    xmm_t b = { .u32 = {0xFFFFFFFF, 0, 200, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmuludq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE00000001 */
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFE00000001ULL,
        "pmuludq max*max: expected 0xFFFFFFFE00000001, got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 20000ULL,
        "pmuludq 100*200: expected 20000, got %lu", dst.u64[1]);
}

static void test_pmuldq_basic(void) {
    xmm_t a = { .i32 = {-1, 0, 100, 0} };
    xmm_t b = { .i32 = {-1, 0, -200, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmuldq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i64[0] == 1, "pmuldq -1*-1=1: got %ld", dst.i64[0]);
    TEST_ASSERT(dst.i64[1] == -20000, "pmuldq 100*-200=-20000: got %ld", dst.i64[1]);
}

static void test_pmulld_basic(void) {
    xmm_t a = { .i32 = {100, -100, 0x7FFFFFFF, -1} };
    xmm_t b = { .i32 = {200, 200, 2, -1} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmulld %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 20000, "pmulld 100*200=20000: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == -20000, "pmulld -100*200=-20000: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[3] == 1, "pmulld -1*-1=1: got %d", dst.i32[3]);
}

int main(void) {
    TEST_START("PMULLW/PMULHW/PMULHUW/PMULDQ/PMULUDQ/PMULLD instructions");
    test_pmullw_basic();
    test_pmullw_overflow();
    test_pmulhw_basic();
    test_pmulhuw_basic();
    test_pmuludq_basic();
    test_pmuldq_basic();
    test_pmulld_basic();
    TEST_END();
}
