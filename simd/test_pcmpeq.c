/*
 * test_pcmpeq.c - Test PCMPEQB/PCMPEQW/PCMPEQD/PCMPEQQ instructions
 *
 * PCMPEQB: Compare packed bytes for equality; set all 1s if equal, all 0s otherwise.
 * PCMPEQW: Compare packed words for equality.
 * PCMPEQD: Compare packed dwords for equality.
 * PCMPEQQ: Compare packed qwords for equality (SSE4.1).
 *
 * Compile: gcc -o test_pcmpeq simd/test_pcmpeq.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pcmpeqb_all_equal(void) {
    xmm_t a = { .u8 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpeqb %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(dst.u8[i] == 0xFF, "pcmpeqb all equal [%d]: expected 0xFF, got 0x%02x", i, dst.u8[i]);
    }
}

static void test_pcmpeqb_mixed(void) {
    xmm_t a = { .u8 = {1,2,3,4, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {1,0,3,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpeqb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0xFF, "pcmpeqb match: got 0x%02x", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 0x00, "pcmpeqb no match: got 0x%02x", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 0xFF, "pcmpeqb match: got 0x%02x", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 0x00, "pcmpeqb no match: got 0x%02x", dst.u8[3]);
}

static void test_pcmpeqw_basic(void) {
    xmm_t a = { .u16 = {100, 200, 300, 400, 500, 600, 700, 800} };
    xmm_t b = { .u16 = {100, 0, 300, 0, 500, 0, 700, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpeqw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0xFFFF, "pcmpeqw match [0]: got 0x%04x", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0x0000, "pcmpeqw no match [1]: got 0x%04x", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0xFFFF, "pcmpeqw match [2]: got 0x%04x", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 0x0000, "pcmpeqw no match [3]: got 0x%04x", dst.u16[3]);
}

static void test_pcmpeqd_basic(void) {
    xmm_t a = { .u32 = {0xDEADBEEF, 0, 0xCAFEBABE, 42} };
    xmm_t b = { .u32 = {0xDEADBEEF, 1, 0xCAFEBABE, 43} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpeqd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "pcmpeqd match: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0x00000000, "pcmpeqd no match: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0xFFFFFFFF, "pcmpeqd match: got 0x%08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0x00000000, "pcmpeqd no match: got 0x%08x", dst.u32[3]);
}

static void test_pcmpeqq_basic(void) {
    xmm_t a = { .u64 = {0xDEADBEEFCAFEBABEULL, 42} };
    xmm_t b = { .u64 = {0xDEADBEEFCAFEBABEULL, 43} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpeqq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL,
        "pcmpeqq match: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x0000000000000000ULL,
        "pcmpeqq no match: got 0x%016lx", dst.u64[1]);
}

static void test_pcmpeqb_boundary(void) {
    xmm_t a = { .u8 = {0, 0xFF, 0x80, 0x7F, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {0, 0xFF, 0x80, 0x7F, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpeqb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0xFF, "pcmpeqb 0==0: got 0x%02x", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 0xFF, "pcmpeqb 0xFF==0xFF: got 0x%02x", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 0xFF, "pcmpeqb 0x80==0x80: got 0x%02x", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 0xFF, "pcmpeqb 0x7F==0x7F: got 0x%02x", dst.u8[3]);
}

static void test_pcmpeqb_mem_operand(void) {
    xmm_t a = { .u8 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    xmm_t b = { .u8 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pcmpeqb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL && dst.u64[1] == 0xFFFFFFFFFFFFFFFFULL,
        "pcmpeqb all equal mem: got 0x%016lx_%016lx", dst.u64[1], dst.u64[0]);
}

int main(void) {
    TEST_START("PCMPEQB/PCMPEQW/PCMPEQD/PCMPEQQ instructions");
    test_pcmpeqb_all_equal();
    test_pcmpeqb_mixed();
    test_pcmpeqw_basic();
    test_pcmpeqd_basic();
    test_pcmpeqq_basic();
    test_pcmpeqb_boundary();
    test_pcmpeqb_mem_operand();
    TEST_END();
}
