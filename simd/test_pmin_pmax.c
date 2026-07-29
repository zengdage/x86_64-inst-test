/*
 * test_pmin_pmax.c - Test PMINSB/PMINSW/PMINSD/PMINUB/PMINUW/PMINUD/
 *                         PMAXSB/PMAXSW/PMAXSD/PMAXUB/PMAXUW/PMAXUD
 *
 * Packed minimum/maximum for signed and unsigned integers at byte/word/dword widths.
 *
 * Compile: gcc -o test_pmin_pmax simd/test_pmin_pmax.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pminsb(void) {
    xmm_t a = { .i8 = {5, -5, 127, -128, 0, 50, -50, 0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {10, -10, -1, 1, 0, -50, 50, 1, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pminsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 5, "pminsb min(5,10)=5: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -10, "pminsb min(-5,-10)=-10: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[2] == -1, "pminsb min(127,-1)=-1: got %d", dst.i8[2]);
    TEST_ASSERT(dst.i8[3] == -128, "pminsb min(-128,1)=-128: got %d", dst.i8[3]);
}

static void test_pmaxsb(void) {
    xmm_t a = { .i8 = {5, -5, 127, -128, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {10, -10, -1, 1, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaxsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 10, "pmaxsb max(5,10)=10: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -5, "pmaxsb max(-5,-10)=-5: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[2] == 127, "pmaxsb max(127,-1)=127: got %d", dst.i8[2]);
    TEST_ASSERT(dst.i8[3] == 1, "pmaxsb max(-128,1)=1: got %d", dst.i8[3]);
}

static void test_pminsw(void) {
    xmm_t a = { .i16 = {100, -100, 32767, -32768, 0,0,0,0} };
    xmm_t b = { .i16 = {-100, 100, -1, 1, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pminsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == -100, "pminsw: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[2] == -1, "pminsw: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -32768, "pminsw: got %d", dst.i16[3]);
}

static void test_pmaxsw(void) {
    xmm_t a = { .i16 = {100, -100, 32767, -32768, 0,0,0,0} };
    xmm_t b = { .i16 = {-100, 100, -1, 1, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaxsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 100, "pmaxsw: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[2] == 32767, "pmaxsw: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 1, "pmaxsw: got %d", dst.i16[3]);
}

static void test_pminsd(void) {
    xmm_t a = { .i32 = {100, -100, 0x7FFFFFFF, (int32_t)0x80000000} };
    xmm_t b = { .i32 = {-100, 100, -1, 1} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pminsd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == -100, "pminsd: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[2] == -1, "pminsd: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == (int32_t)0x80000000, "pminsd: got %d", dst.i32[3]);
}

static void test_pmaxsd(void) {
    xmm_t a = { .i32 = {100, -100, 0x7FFFFFFF, (int32_t)0x80000000} };
    xmm_t b = { .i32 = {-100, 100, -1, 1} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaxsd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 100, "pmaxsd: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[2] == 0x7FFFFFFF, "pmaxsd: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 1, "pmaxsd: got %d", dst.i32[3]);
}

static void test_pminub(void) {
    xmm_t a = { .u8 = {0, 128, 255, 100, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {1, 127, 0, 200, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pminub %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0, "pminub: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 127, "pminub: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 0, "pminub: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 100, "pminub: got %u", dst.u8[3]);
}

static void test_pmaxub(void) {
    xmm_t a = { .u8 = {0, 128, 255, 100, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {1, 127, 0, 200, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaxub %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 1, "pmaxub: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 128, "pmaxub: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 255, "pmaxub: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 200, "pmaxub: got %u", dst.u8[3]);
}

static void test_pminuw(void) {
    xmm_t a = { .u16 = {0, 0x8000, 0xFFFF, 100, 0,0,0,0} };
    xmm_t b = { .u16 = {1, 0x7FFF, 0, 200, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pminuw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "pminuw: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0x7FFF, "pminuw: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0, "pminuw: got %u", dst.u16[2]);
}

static void test_pminud(void) {
    xmm_t a = { .u32 = {0, 0x80000000, 0xFFFFFFFF, 100} };
    xmm_t b = { .u32 = {1, 0x7FFFFFFF, 0, 200} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pminud %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0, "pminud: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0x7FFFFFFF, "pminud: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0, "pminud: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 100, "pminud: got %u", dst.u32[3]);
}

static void test_pmaxud(void) {
    xmm_t a = { .u32 = {0, 0x80000000, 0xFFFFFFFF, 100} };
    xmm_t b = { .u32 = {1, 0x7FFFFFFF, 0, 200} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmaxud %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 1, "pmaxud: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0x80000000, "pmaxud: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0xFFFFFFFF, "pmaxud: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 200, "pmaxud: got %u", dst.u32[3]);
}

int main(void) {
    TEST_START("PMIN/PMAX instructions");
    test_pminsb();
    test_pmaxsb();
    test_pminsw();
    test_pmaxsw();
    test_pminsd();
    test_pmaxsd();
    test_pminub();
    test_pmaxub();
    test_pminuw();
    test_pminud();
    test_pmaxud();
    TEST_END();
}
