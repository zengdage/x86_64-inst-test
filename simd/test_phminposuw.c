/*
 * test_phminposuw.c - Test SSE4.1 PHMINPOSUW instruction
 *
 * PHMINPOSUW: Find the horizontal minimum of the 8 packed unsigned 16-bit words
 *             in the source xmm. dst[15:0] = min value, dst[18:16] = index of the
 *             minimum (lowest index wins ties), dst[127:19] = 0.
 *
 * Compile: gcc -o test_phminposuw simd/test_phminposuw.c -O2 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_phminposuw_basic(void) {
    xmm_t src = { .u16 = {100, 50, 200, 30, 250, 10, 175, 90} };
    xmm_t r;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phminposuw %%xmm0, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(r) : "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u16[0] == 10u, "phminposuw value: expected 10, got %u", r.u16[0]);
    TEST_ASSERT(r.u16[1] == 5u,  "phminposuw index: expected 5, got %u",  r.u16[1]);
    /* upper bits must be zero */
    for (int i = 2; i < 8; i++) {
        TEST_ASSERT(r.u16[i] == 0u, "phminposuw [%d] zeroed: got %u", i, r.u16[i]);
    }
}

static void test_phminposuw_first_index_wins(void) {
    /* multiple equal minima: lowest index wins */
    xmm_t src = { .u16 = {7, 7, 7, 7, 1, 1, 9, 9} };
    xmm_t r;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phminposuw %%xmm0, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(r) : "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u16[0] == 1u, "phminposuw value: expected 1, got %u", r.u16[0]);
    TEST_ASSERT(r.u16[1] == 4u, "phminposuw index: expected 4 (first of ties), got %u", r.u16[1]);
}

static void test_phminposuw_first_lane(void) {
    xmm_t src = { .u16 = {1, 100, 200, 300, 400, 500, 600, 700} };
    xmm_t r;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phminposuw %%xmm0, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(r) : "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u16[0] == 1u, "phminposuw value: expected 1, got %u", r.u16[0]);
    TEST_ASSERT(r.u16[1] == 0u, "phminposuw index: expected 0, got %u", r.u16[1]);
}

static void test_phminposuw_last_lane(void) {
    xmm_t src = { .u16 = {100, 200, 300, 400, 500, 600, 700, 1} };
    xmm_t r;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phminposuw %%xmm0, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(r) : "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u16[0] == 1u, "phminposuw value: expected 1, got %u", r.u16[0]);
    TEST_ASSERT(r.u16[1] == 7u, "phminposuw index: expected 7, got %u", r.u16[1]);
}

static void test_phminposuw_max_values(void) {
    /* All 0xFFFF — all equal minima; index 0 should win */
    xmm_t src = { .u16 = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF} };
    xmm_t r;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phminposuw %%xmm0, %%xmm1\n\t"
        "movdqa %%xmm1, %0"
        : "=m"(r) : "m"(src) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.u16[0] == 0xFFFFu, "phminposuw value: expected 0xFFFF, got 0x%x", r.u16[0]);
    TEST_ASSERT(r.u16[1] == 0u,      "phminposuw index: expected 0, got %u",       r.u16[1]);
}

static void test_phminposuw_mem_operand(void) {
    xmm_t src = { .u16 = {100, 50, 200, 30, 250, 10, 175, 90} };
    xmm_t r;
    __asm__ volatile (
        "phminposuw %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(r.u16[0] == 10u, "phminposuw mem value: expected 10, got %u", r.u16[0]);
    TEST_ASSERT(r.u16[1] == 5u,  "phminposuw mem index: expected 5, got %u",  r.u16[1]);
}

int main(void) {
    TEST_START("PHMINPOSUW");
    test_phminposuw_basic();
    test_phminposuw_first_index_wins();
    test_phminposuw_first_lane();
    test_phminposuw_last_lane();
    test_phminposuw_max_values();
    test_phminposuw_mem_operand();
    TEST_END();
}
