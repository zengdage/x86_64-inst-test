/*
 * test_pmovzx.c - Test PMOVZXBD/PMOVZXBQ/PMOVZXDQ/PMOVZXWD/PMOVZXWQ instructions
 *
 * Zero-extend packed integers to wider types (SSE4.1).
 * PMOVZXBD: Packed zero-extend bytes to dwords (4 bytes -> 4 dwords).
 * PMOVZXBQ: Packed zero-extend bytes to qwords (2 bytes -> 2 qwords).
 * PMOVZXWD: Packed zero-extend words to dwords (4 words -> 4 dwords).
 * PMOVZXWQ: Packed zero-extend words to qwords (2 words -> 2 qwords).
 * PMOVZXDQ: Packed zero-extend dwords to qwords (2 dwords -> 2 qwords).
 *
 * Compile: gcc -o test_pmovzx simd/test_pmovzx.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pmovzxbd(void) {
    xmm_t src = { .u8 = {0, 1, 128, 255, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovzxbd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0, "pmovzxbd [0]: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 1, "pmovzxbd [1]: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 128, "pmovzxbd [2]: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 255, "pmovzxbd [3]: got %u", dst.u32[3]);
}

static void test_pmovzxbq(void) {
    xmm_t src = { .u8 = {0xFF, 0x80, 0,0,0,0,0,0, 0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovzxbq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 255, "pmovzxbq [0]: got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 128, "pmovzxbq [1]: got %lu", dst.u64[1]);
}

static void test_pmovzxwd(void) {
    xmm_t src = { .u16 = {0, 1, 0x8000, 0xFFFF, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovzxwd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0, "pmovzxwd [0]: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 1, "pmovzxwd [1]: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0x8000, "pmovzxwd [2]: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0xFFFF, "pmovzxwd [3]: got %u", dst.u32[3]);
}

static void test_pmovzxwq(void) {
    xmm_t src = { .u16 = {0xFFFF, 0x1234, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovzxwq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFF, "pmovzxwq [0]: got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x1234, "pmovzxwq [1]: got %lu", dst.u64[1]);
}

static void test_pmovzxdq(void) {
    xmm_t src = { .u32 = {0xFFFFFFFF, 0x12345678, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovzxdq %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFF, "pmovzxdq [0]: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x12345678, "pmovzxdq [1]: got 0x%016lx", dst.u64[1]);
}

static void test_pmovzxbd_all_max(void) {
    xmm_t src = { .u8 = {255, 255, 255, 255, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "pmovzxbd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.u32[i] == 255, "pmovzxbd max [%d]: got %u", i, dst.u32[i]);
    }
}

int main(void) {
    TEST_START("PMOVZXBD/PMOVZXBQ/PMOVZXDQ/PMOVZXWD/PMOVZXWQ instructions");
    test_pmovzxbd();
    test_pmovzxbq();
    test_pmovzxwd();
    test_pmovzxwq();
    test_pmovzxdq();
    test_pmovzxbd_all_max();
    TEST_END();
}
