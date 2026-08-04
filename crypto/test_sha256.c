/*
 * test_sha256.c - Test x86-64 SHA-256 instructions
 *
 * SHA256RNDS2 xmm1, xmm2/m128, <xmm0>
 *   Performs two rounds of SHA-256 operation.
 *   The implicit xmm0 operand contains the round message/constant data.
 *
 * SHA256MSG1 xmm1, xmm2/m128
 *   Performs an intermediate calculation for the next four SHA-256 message dwords.
 *   Applies sigma0 function.
 *
 * SHA256MSG2 xmm1, xmm2/m128
 *   Performs the final calculation for the next four SHA-256 message dwords.
 *   Applies sigma1 function.
 *
 * Requires SHA extension support.
 *
 * Compile: gcc -o test_sha256 crypto/test_sha256.c -O0 -msha
 * Note: Do not use static linking.
 */
#include "../common.h"

/* Test SHA256RNDS2 basic operation */
static void test_sha256rnds2_basic(void) {
    /* SHA-256 initial hash values (first 4 and last 4 of H0-H7) */
    xmm_t cdgh = { .u32 = {0x5be0cd19, 0x1f83d9ab, 0x9b05688c, 0x510e527f} };
    xmm_t abef = { .u32 = {0xe9b5dba5, 0xb5c0fbcf, 0x71374491, 0x6a09e667} };
    /* Message schedule + round constant for first two rounds */
    xmm_t wk   = { .u32 = {0x5a827999, 0x6ed9eba1, 0, 0} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm1\n\t"
        "movdqu %2, %%xmm2\n\t"
        "movdqu %3, %%xmm0\n\t"
        "sha256rnds2 %%xmm2, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result)
        : "m"(cdgh), "m"(abef), "m"(wk)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(memcmp(&result, &cdgh, 16) != 0,
                "sha256rnds2: output differs from input state");
    const xmm_t expected = { .u32 = {0xa448e70d,0xc0b83fca,0x48737976,0x6c2896a9} };
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0, "sha256rnds2 golden output");

    /* Verify determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm1\n\t"
        "movdqu %2, %%xmm2\n\t"
        "movdqu %3, %%xmm0\n\t"
        "sha256rnds2 %%xmm2, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result2)
        : "m"(cdgh), "m"(abef), "m"(wk)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "sha256rnds2: deterministic");
}

/* Test SHA256RNDS2 xmm, mem */
static void test_sha256rnds2_mem(void) {
    xmm_t cdgh = { .u32 = {0x5be0cd19, 0x1f83d9ab, 0x9b05688c, 0x510e527f} };
    xmm_t abef = { .u32 = {0xe9b5dba5, 0xb5c0fbcf, 0x71374491, 0x6a09e667} };
    xmm_t wk   = { .u32 = {0x428a2f98, 0x71374491, 0, 0} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm1\n\t"
        "movdqu %2, %%xmm2\n\t"
        "movdqu %3, %%xmm0\n\t"
        "sha256rnds2 %%xmm2, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_reg)
        : "m"(cdgh), "m"(abef), "m"(wk)
        : "xmm0", "xmm1", "xmm2"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm1\n\t"
        "movdqu %3, %%xmm0\n\t"
        "sha256rnds2 %2, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_mem)
        : "m"(cdgh), "m"(abef), "m"(wk)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "sha256rnds2: reg-reg and reg-mem produce same result");
}

/* Test SHA256RNDS2 with different message constants */
static void test_sha256rnds2_different_wk(void) {
    xmm_t state1 = { .u32 = {0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321} };
    xmm_t state2 = { .u32 = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD} };
    xmm_t wk1 = { .u32 = {0x11111111, 0x22222222, 0, 0} };
    xmm_t wk2 = { .u32 = {0x33333333, 0x44444444, 0, 0} };
    xmm_t result1, result2;

    __asm__ volatile (
        "movdqu %1, %%xmm1\n\t"
        "movdqu %2, %%xmm2\n\t"
        "movdqu %3, %%xmm0\n\t"
        "sha256rnds2 %%xmm2, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result1)
        : "m"(state1), "m"(state2), "m"(wk1)
        : "xmm0", "xmm1", "xmm2"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm1\n\t"
        "movdqu %2, %%xmm2\n\t"
        "movdqu %3, %%xmm0\n\t"
        "sha256rnds2 %%xmm2, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result2)
        : "m"(state1), "m"(state2), "m"(wk2)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(memcmp(&result1, &result2, 16) != 0,
                "sha256rnds2: different W+K values produce different results");
}

/* Test SHA256MSG1 */
static void test_sha256msg1(void) {
    xmm_t w0_3 = { .u32 = {0x61626380, 0x12345678, 0x9ABCDEF0, 0xCAFEBABE} };
    xmm_t w4_7 = { .u32 = {0xDEADBEEF, 0x11111111, 0x22222222, 0x33333333} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha256msg1 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(w0_3), "m"(w4_7)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &w0_3, 16) != 0,
                "sha256msg1: output differs from input");
    const xmm_t expected = { .u32 = {0x495f4a6e,0xd8131b44,0x6522778d,0x76d1d5c9} };
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0, "sha256msg1 golden output");

    /* Determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha256msg1 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(w0_3), "m"(w4_7)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "sha256msg1: deterministic");
}

/* Test SHA256MSG1 xmm, mem */
static void test_sha256msg1_mem(void) {
    xmm_t a = { .u32 = {0x11111111, 0x22222222, 0x33333333, 0x44444444} };
    xmm_t b = { .u32 = {0x55555555, 0x66666666, 0x77777777, 0x88888888} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha256msg1 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "sha256msg1 %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(a), "m"(b)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "sha256msg1: reg-reg and reg-mem produce same result");
}

/* Test SHA256MSG2 */
static void test_sha256msg2(void) {
    xmm_t a = { .u32 = {0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321} };
    xmm_t b = { .u32 = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha256msg2 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &a, 16) != 0,
                "sha256msg2: output differs from input");
    const xmm_t expected = { .u32 = {0x12012344,0xf01f0112,0xc5bbd6b1,0xe7fc64ed} };
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0, "sha256msg2 golden output");

    /* Determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha256msg2 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "sha256msg2: deterministic");
}

/* Test SHA256MSG2 xmm, mem */
static void test_sha256msg2_mem(void) {
    xmm_t a = { .u32 = {0x11111111, 0x22222222, 0x33333333, 0x44444444} };
    xmm_t b = { .u32 = {0x55555555, 0x66666666, 0x77777777, 0x88888888} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha256msg2 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "sha256msg2 %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(a), "m"(b)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "sha256msg2: reg-reg and reg-mem produce same result");
}

/* Test SHA256MSG1 and SHA256MSG2 together for message schedule */
static void test_sha256_msg_schedule(void) {
    /* Simulate message schedule expansion W[16] from W[0..15] */
    xmm_t w0  = { .u32 = {0x61626380, 0x00000000, 0x00000000, 0x00000000} };
    xmm_t w4  = { .u32 = {0x00000000, 0x00000000, 0x00000000, 0x00000000} };
    xmm_t w8  = { .u32 = {0x00000000, 0x00000000, 0x00000000, 0x00000000} };
    xmm_t w12 = { .u32 = {0x00000000, 0x00000000, 0x00000000, 0x00000018} };
    xmm_t w16;

    /* W[16..19] = sha256msg2(sha256msg1(W[0..3], W[4..7]) + W[8..11], W[12..15]) */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"     /* W[0..3] */
        "movdqu %2, %%xmm1\n\t"     /* W[4..7] */
        "sha256msg1 %%xmm1, %%xmm0\n\t"
        "movdqu %3, %%xmm2\n\t"     /* W[8..11] */
        "paddd %%xmm2, %%xmm0\n\t"
        "movdqu %4, %%xmm3\n\t"     /* W[12..15] */
        "sha256msg2 %%xmm3, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(w16)
        : "m"(w0), "m"(w4), "m"(w8), "m"(w12)
        : "xmm0", "xmm1", "xmm2", "xmm3"
    );

    /* Result should be non-trivial */
    TEST_ASSERT(w16.u64[0] != 0 || w16.u64[1] != 0,
                "sha256 msg schedule: W[16..19] non-zero for 'abc' padding");
}

int main(void) {
    TEST_START("SHA256 instructions (SHA256RNDS2/SHA256MSG1/SHA256MSG2)");
    test_sha256rnds2_basic();
    test_sha256rnds2_mem();
    test_sha256rnds2_different_wk();
    test_sha256msg1();
    test_sha256msg1_mem();
    test_sha256msg2();
    test_sha256msg2_mem();
    test_sha256_msg_schedule();
    TEST_END();
}
