/*
 * test_aeskeygenassist.c - Test x86-64 AESKEYGENASSIST instruction
 *
 * AESKEYGENASSIST assists in AES round key generation.
 * dst = AESKEYGENASSIST(src, imm8)
 * For each 32-bit column X of src:
 *   Apply SubWord (SubBytes to each byte of the word)
 *   Then RotWord on the high column
 *   Then XOR the RCON value from imm8 onto specific positions
 *
 * Specifically:
 *   X1 = src[31:0], X3 = src[95:64]
 *   dst[31:0]   = SubWord(X1)
 *   dst[63:32]  = RotWord(SubWord(X1)) XOR RCON
 *   dst[95:64]  = SubWord(X3)
 *   dst[127:96] = RotWord(SubWord(X3)) XOR RCON
 *
 * Requires AES-NI support.
 *
 * Compile: gcc -o test_aeskeygenassist crypto/test_aeskeygenassist.c -O0 -maes
 * Note: Do not use static linking.
 */
#include "../common.h"
#include "aes_ref.h"

/* Test AESKEYGENASSIST xmm, xmm, imm8 with zero input */
static void test_aeskeygenassist_zero(void) {
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "aeskeygenassist $0x00, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result)
        :
        : "xmm0", "xmm1"
    );
    aes_ref_keygenassist((const uint8_t[16]){0}, 0x00, expected.u8);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "aeskeygenassist zero/rcon0 matches independent reference");

    /* SubBytes(0x00) = 0x63
     * SubWord(0x00000000) = 0x63636363
     * RotWord(0x63636363) = 0x63636363
     * With RCON=0: dst = [63636363, 63636363, 63636363, 63636363] */
    TEST_ASSERT(result.u32[0] == 0x63636363,
                "aeskeygenassist(0, 0x00): word0 = 0x%08x (expected 0x63636363)",
                result.u32[0]);
    TEST_ASSERT(result.u32[1] == 0x63636363,
                "aeskeygenassist(0, 0x00): word1 = 0x%08x (expected 0x63636363)",
                result.u32[1]);
    TEST_ASSERT(result.u32[2] == 0x63636363,
                "aeskeygenassist(0, 0x00): word2 = 0x%08x (expected 0x63636363)",
                result.u32[2]);
    TEST_ASSERT(result.u32[3] == 0x63636363,
                "aeskeygenassist(0, 0x00): word3 = 0x%08x (expected 0x63636363)",
                result.u32[3]);
}

/* Test AESKEYGENASSIST with RCON=0x01 */
static void test_aeskeygenassist_rcon01(void) {
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "aeskeygenassist $0x01, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result)
        :
        : "xmm0", "xmm1"
    );
    aes_ref_keygenassist((const uint8_t[16]){0}, 0x01, expected.u8);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "aeskeygenassist zero/rcon1 matches independent reference");

    /* SubWord(0) = 0x63636363
     * RotWord(0x63636363) = 0x63636363
     * XOR RCON: 0x63636363 ^ 0x01000000 = 0x62636363 (little-endian: 0x63636362)
     * Wait, RCON goes into byte 0 in Intel convention: 0x63636363 ^ 0x00000001 = 0x63636362
     * Actually the RCON is XOR'd with the least-significant byte in little-endian */
    /* word0 = SubWord(X1) = 0x63636363 (no RCON) */
    TEST_ASSERT(result.u32[0] == 0x63636363,
                "aeskeygenassist(0, 0x01): word0 = 0x%08x (expected 0x63636363)",
                result.u32[0]);
    /* word1 = RotWord(SubWord(X1)) ^ RCON */
    /* RotWord(0x63636363) = 0x63636363, RCON goes in byte 0 */
    /* 0x63636363 ^ 0x00000001 = 0x63636362 */
    TEST_ASSERT((result.u32[1] & 0xFF) == 0x62,
                "aeskeygenassist(0, 0x01): word1 low byte has RCON applied");
}

/* Test AESKEYGENASSIST xmm, mem */
static void test_aeskeygenassist_mem(void) {
    xmm_t input = { .u8 = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
                            0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c} };
    xmm_t result_reg, result_mem;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aeskeygenassist $0x01, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_reg)
        : "m"(input)
        : "xmm0", "xmm1"
    );
    aes_ref_keygenassist(input.u8, 0x01, expected.u8);

    __asm__ volatile (
        "aeskeygenassist $0x01, %1, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_mem)
        : "m"(input)
        : "xmm1"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "aeskeygenassist: reg and mem source produce same result");
    TEST_ASSERT(memcmp(&result_mem, &expected, 16) == 0,
                "aeskeygenassist memory source matches independent reference");
}

/* Test AESKEYGENASSIST with known AES-128 key expansion vector
 * Key: 2b7e151628aed2a6abf7158809cf4f3c (FIPS-197 Appendix A.1) */
static void test_aeskeygenassist_aes128_key(void) {
    /* The AES-128 key in little-endian xmm layout */
    xmm_t key = { .u8 = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
                          0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c} };
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aeskeygenassist $0x01, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result)
        : "m"(key)
        : "xmm0", "xmm1"
    );
    aes_ref_keygenassist(key.u8, 0x01, expected.u8);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "aeskeygenassist AES-128 key matches independent reference");

    /* Verify result is deterministic and non-trivial */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aeskeygenassist $0x01, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result2)
        : "m"(key)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "aeskeygenassist with AES-128 key: deterministic");
    TEST_ASSERT(memcmp(&result, &key, 16) != 0,
                "aeskeygenassist with AES-128 key: output differs from input");
}

/* Test different RCON values produce different results */
static void test_aeskeygenassist_different_rcons(void) {
    xmm_t input = { .u8 = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                            0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10} };
    xmm_t result_r01, result_r02;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aeskeygenassist $0x01, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_r01)
        : "m"(input)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aeskeygenassist $0x02, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_r02)
        : "m"(input)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result_r01, &result_r02, 16) != 0,
                "aeskeygenassist: different RCON values produce different results");

    /* Words 0 and 2 should be the same (RCON only affects words 1 and 3) */
    TEST_ASSERT(result_r01.u32[0] == result_r02.u32[0],
                "aeskeygenassist: word0 unaffected by RCON");
    TEST_ASSERT(result_r01.u32[2] == result_r02.u32[2],
                "aeskeygenassist: word2 unaffected by RCON");
}

/* Test that only dwords 1 and 3 of the source matter
 * (X1 = src[63:32], X3 = src[127:96]) */
static void test_aeskeygenassist_source_words(void) {
    /* Same dwords 1 and 3, different dwords 0 and 2 */
    xmm_t input1 = { .u32 = {0xAAAAAAAA, 0x01020304, 0xBBBBBBBB, 0x05060708} };
    xmm_t input2 = { .u32 = {0xCCCCCCCC, 0x01020304, 0xDDDDDDDD, 0x05060708} };
    xmm_t result1, result2;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aeskeygenassist $0x01, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result1)
        : "m"(input1)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aeskeygenassist $0x01, %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result2)
        : "m"(input2)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result1, &result2, 16) == 0,
                "aeskeygenassist: only dwords 1 and 3 of source matter");
}

int main(void) {
    TEST_START("AESKEYGENASSIST instruction");
    test_aeskeygenassist_zero();
    test_aeskeygenassist_rcon01();
    test_aeskeygenassist_mem();
    test_aeskeygenassist_aes128_key();
    test_aeskeygenassist_different_rcons();
    test_aeskeygenassist_source_words();
    TEST_END();
}
