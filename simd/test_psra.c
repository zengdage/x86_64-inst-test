/*
 * test_psra.c - Test PSRAW/PSRAD instructions
 *
 * PSRAW: Packed shift right arithmetic words (sign-fill).
 * PSRAD: Packed shift right arithmetic doublewords (sign-fill).
 * Note: There is no PSRAQ in SSE (only available in AVX-512).
 * If count > element size, result is 0 or all 1s depending on sign.
 *
 * Compile: gcc -o test_psra simd/test_psra.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_psraw_positive(void) {
    xmm_t a = { .i16 = {0x7FFF, 256, 16, 1, 0, 0, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psraw $4, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 0x07FF, "psraw 0x7FFF>>4=0x07FF: got 0x%04x", (uint16_t)dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 16, "psraw 256>>4=16: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 1, "psraw 16>>4=1: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 0, "psraw 1>>4=0: got %d", dst.i16[3]);
}

static void test_psraw_negative(void) {
    xmm_t a = { .i16 = {-1, -16, -256, (int16_t)0x8000, 0, 0, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psraw $4, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == -1, "psraw -1>>4=-1 (sign-extended): got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -1, "psraw -16>>4=-1: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == -16, "psraw -256>>4=-16: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == (int16_t)0xF800, "psraw 0x8000>>4=0xF800: got 0x%04x", (uint16_t)dst.i16[3]);
}

static void test_psraw_count_exceeds(void) {
    xmm_t a = { .i16 = {0x7FFF, (int16_t)0x8000, 0, 0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psraw $16, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 0, "psraw positive>>16=0: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -1, "psraw negative>>16=-1: got %d", dst.i16[1]);
}

static void test_psrad_positive(void) {
    xmm_t a = { .i32 = {0x7FFFFFFF, 256, 1, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrad $8, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 0x007FFFFF, "psrad 0x7FFFFFFF>>8: got 0x%08x", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 1, "psrad 256>>8=1: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 0, "psrad 1>>8=0: got %d", dst.i32[2]);
}

static void test_psrad_negative(void) {
    xmm_t a = { .i32 = {-1, -256, (int32_t)0x80000000, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrad $8, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == -1, "psrad -1>>8=-1: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == -1, "psrad -256>>8=-1: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == (int32_t)0xFF800000, "psrad 0x80000000>>8: got 0x%08x", dst.i32[2]);
}

static void test_psrad_xmm_count(void) {
    xmm_t a = { .i32 = {256, -256, 0, 0} };
    xmm_t count = { .u64 = {4, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "psrad %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(count) : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.i32[0] == 16, "psrad xmm count 256>>4=16: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == -16, "psrad xmm count -256>>4=-16: got %d", dst.i32[1]);
}

static void test_psraw_zero_shift(void) {
    xmm_t a = { .i16 = {-1234, 5678, 0, 0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psraw $0, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == -1234, "psraw shift 0 unchanged: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 5678, "psraw shift 0 unchanged: got %d", dst.i16[1]);
}

static void test_psra_boundaries(void) {
    xmm_t a = { .i32 = {INT32_MIN, INT32_MAX, -1, 0} };
    xmm_t dst;
    xmm_t count = { .u64 = {1, UINT64_MAX} };

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrad $31, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == -1 && dst.i32[1] == 0 &&
                dst.i32[2] == -1 && dst.i32[3] == 0,
                "psrad last valid count 31 saturates to sign");

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrad $0xff, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == -1 && dst.i32[1] == 0 &&
                dst.i32[2] == -1 && dst.i32[3] == 0,
                "psrad max imm8 saturates to sign");

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrad %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(count) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == INT32_MIN / 2 && dst.i32[1] == INT32_MAX / 2,
                "psrad vector count ignores upper qword");
}

int main(void) {
    TEST_START("PSRAW/PSRAD instructions");
    test_psraw_positive();
    test_psraw_negative();
    test_psraw_count_exceeds();
    test_psrad_positive();
    test_psrad_negative();
    test_psrad_xmm_count();
    test_psraw_zero_shift();
    test_psra_boundaries();
    TEST_END();
}
