/*
 * test_pcmpgt.c - Test PCMPGTB/PCMPGTW/PCMPGTD/PCMPGTQ instructions
 *
 * PCMPGTB: Compare packed signed bytes for greater-than.
 * PCMPGTW: Compare packed signed words for greater-than.
 * PCMPGTD: Compare packed signed dwords for greater-than.
 * PCMPGTQ: Compare packed signed qwords for greater-than (SSE4.2).
 * Result: all 1s if dest > src (signed), all 0s otherwise.
 *
 * Compile: gcc -o test_pcmpgt simd/test_pcmpgt.c -O0 -msse4.2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pcmpgtb_basic(void) {
    xmm_t a = { .i8 = {5, -5, 0, 127, -128, 10, 0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {4, -6, 0, 126, -127, -10, 0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpgtb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0xFF, "pcmpgtb 5>4: got 0x%02x", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 0xFF, "pcmpgtb -5>-6: got 0x%02x", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 0x00, "pcmpgtb 0==0: got 0x%02x", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 0xFF, "pcmpgtb 127>126: got 0x%02x", dst.u8[3]);
    TEST_ASSERT(dst.u8[4] == 0x00, "pcmpgtb -128<-127: got 0x%02x", dst.u8[4]);
    TEST_ASSERT(dst.u8[5] == 0xFF, "pcmpgtb 10>-10: got 0x%02x", dst.u8[5]);
}

static void test_pcmpgtw_basic(void) {
    xmm_t a = { .i16 = {100, -100, 0, 32767, -32768, 0, 0, 0} };
    xmm_t b = { .i16 = {99, -101, 0, 32766, -32767, 0, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpgtw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0xFFFF, "pcmpgtw 100>99: got 0x%04x", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0xFFFF, "pcmpgtw -100>-101: got 0x%04x", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0x0000, "pcmpgtw 0==0: got 0x%04x", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 0xFFFF, "pcmpgtw 32767>32766: got 0x%04x", dst.u16[3]);
    TEST_ASSERT(dst.u16[4] == 0x0000, "pcmpgtw -32768<-32767: got 0x%04x", dst.u16[4]);
}

static void test_pcmpgtd_basic(void) {
    xmm_t a = { .i32 = {1000, -1000, 0x7FFFFFFF, (int32_t)0x80000000} };
    xmm_t b = { .i32 = {999, -1001, 0x7FFFFFFE, (int32_t)0x80000001} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpgtd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "pcmpgtd 1000>999: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0xFFFFFFFF, "pcmpgtd -1000>-1001: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0xFFFFFFFF, "pcmpgtd INT_MAX>INT_MAX-1: got 0x%08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0x00000000, "pcmpgtd INT_MIN<INT_MIN+1: got 0x%08x", dst.u32[3]);
}

static void test_pcmpgtq_basic(void) {
    xmm_t a = { .i64 = {100, -100} };
    xmm_t b = { .i64 = {99, -99} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpgtq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL, "pcmpgtq 100>99: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x0000000000000000ULL, "pcmpgtq -100<-99: got 0x%016lx", dst.u64[1]);
}

static void test_pcmpgtq_equal(void) {
    xmm_t a = { .i64 = {42, -42} };
    xmm_t b = { .i64 = {42, -42} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpgtq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0, "pcmpgtq 42==42: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "pcmpgtq -42==-42: got 0x%016lx", dst.u64[1]);
}

int main(void) {
    TEST_START("PCMPGTB/PCMPGTW/PCMPGTD/PCMPGTQ instructions");
    test_pcmpgtb_basic();
    test_pcmpgtw_basic();
    test_pcmpgtd_basic();
    test_pcmpgtq_basic();
    test_pcmpgtq_equal();
    TEST_END();
}
