/*
 * test_pmovsx.c - Test PMOVSXBW/PMOVSXBD/PMOVSXBQ/PMOVSXDQ/PMOVSXWD/PMOVSXWQ
 *
 * Sign-extend packed integers to wider types (SSE4.1).
 * PMOVSXBW: Packed sign-extend bytes to words (8 bytes -> 8 words).
 * PMOVSXBD: Packed sign-extend bytes to dwords (4 bytes -> 4 dwords).
 * PMOVSXBQ: Packed sign-extend bytes to qwords (2 bytes -> 2 qwords).
 * PMOVSXWD: Packed sign-extend words to dwords (4 words -> 4 dwords).
 * PMOVSXWQ: Packed sign-extend words to qwords (2 words -> 2 qwords).
 * PMOVSXDQ: Packed sign-extend dwords to qwords (2 dwords -> 2 qwords).
 *
 * Compile: gcc -o test_pmovsx sse/test_pmovsx.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pmovsxbw(void) {
    const xmm_t src = { .i8 = {
        INT8_MIN, -127, -1, 0, 1, 42, 126, INT8_MAX,
        99, 98, 97, 96, 95, 94, 93, 92
    } };
    xmm_t dst;

    __asm__ volatile (
        "pmovsxbw %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i16[i] == (int16_t)src.i8[i],
                    "pmovsxbw memory lane %d: got %d", i, dst.i16[i]);

    __asm__ volatile (
        "movdqu %1, %%xmm1\n\t"
        "pmovsxbw %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0", "xmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i16[i] == (int16_t)src.i8[i],
                    "pmovsxbw register lane %d: got %d", i, dst.i16[i]);
}

static void test_pmovsxbd(void) {
    xmm_t src = { .i8 = {1, -1, 127, -128, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovsxbd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 1, "pmovsxbd [0]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == -1, "pmovsxbd [1]: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 127, "pmovsxbd [2]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == -128, "pmovsxbd [3]: got %d", dst.i32[3]);
}

static void test_pmovsxbq(void) {
    xmm_t src = { .i8 = {42, -42, 0,0,0,0,0,0, 0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovsxbq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.i64[0] == 42, "pmovsxbq [0]: got %ld", dst.i64[0]);
    TEST_ASSERT(dst.i64[1] == -42, "pmovsxbq [1]: got %ld", dst.i64[1]);
}

static void test_pmovsxwd(void) {
    xmm_t src = { .i16 = {100, -100, 32767, -32768, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovsxwd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 100, "pmovsxwd [0]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == -100, "pmovsxwd [1]: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 32767, "pmovsxwd [2]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == -32768, "pmovsxwd [3]: got %d", dst.i32[3]);
}

static void test_pmovsxwq(void) {
    xmm_t src = { .i16 = {1000, -1000, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovsxwq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.i64[0] == 1000, "pmovsxwq [0]: got %ld", dst.i64[0]);
    TEST_ASSERT(dst.i64[1] == -1000, "pmovsxwq [1]: got %ld", dst.i64[1]);
}

static void test_pmovsxdq(void) {
    xmm_t src = { .i32 = {0x7FFFFFFF, (int32_t)0x80000000, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovsxdq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.i64[0] == 0x7FFFFFFF, "pmovsxdq [0]: got %ld", dst.i64[0]);
    TEST_ASSERT(dst.i64[1] == (int64_t)(int32_t)0x80000000, "pmovsxdq [1]: got %ld", dst.i64[1]);
}

static void test_pmovsxbd_zero(void) {
    xmm_t src = { .i8 = {0, 0, 0, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovsxbd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.i32[i] == 0, "pmovsxbd zero [%d]: got %d", i, dst.i32[i]);
    }
}

int main(void) {
    TEST_START("PMOVSXBW/PMOVSXBD/PMOVSXBQ/PMOVSXDQ/PMOVSXWD/PMOVSXWQ instructions");
    test_pmovsxbw();
    test_pmovsxbd();
    test_pmovsxbq();
    test_pmovsxwd();
    test_pmovsxwq();
    test_pmovsxdq();
    test_pmovsxbd_zero();
    TEST_END();
}
