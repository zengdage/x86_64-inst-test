/*
 * test_aesimc.c - Test x86-64 AESIMC instruction
 *
 * AESIMC performs the AES InverseMixColumns transformation.
 * It is used to convert encryption round keys to decryption round keys.
 * dst = InvMixColumns(src)
 *
 * Operates on 128-bit XMM registers.
 * Requires AES-NI support.
 *
 * Compile: gcc -o test_aesimc crypto/test_aesimc.c -O0 -maes
 * Note: Do not use static linking.
 */
#include "../common.h"
#include "aes_ref.h"

/* Test AESIMC xmm, xmm */
static void test_aesimc_reg(void) {
    xmm_t input = { .u8 = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,
                            0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00} };
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result)
        : "m"(input)
        : "xmm0", "xmm1"
    );
    aes_ref_inv_mix(input.u8, expected.u8);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "aesimc xmm,xmm matches independent InvMixColumns reference");

    /* Verify determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result2)
        : "m"(input)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "aesimc xmm,xmm: deterministic");

    TEST_ASSERT(memcmp(&result, &input, 16) != 0,
                "aesimc xmm,xmm: output differs from input");
}

/* Test AESIMC xmm, mem */
static void test_aesimc_mem(void) {
    xmm_t input = { .u8 = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
                            0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c} };
    xmm_t result_reg, result_mem;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_reg)
        : "m"(input)
        : "xmm0", "xmm1"
    );
    aes_ref_inv_mix(input.u8, expected.u8);

    __asm__ volatile (
        "aesimc %1, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result_mem)
        : "m"(input)
        : "xmm1"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "aesimc: reg and mem source produce same result");
    TEST_ASSERT(memcmp(&result_mem, &expected, 16) == 0,
                "aesimc memory source matches independent reference");
}

/* Test AESIMC with zero input */
static void test_aesimc_zero(void) {
    xmm_t result;

    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result)
        :
        : "xmm0", "xmm1"
    );

    /* InvMixColumns of all zeros should be all zeros */
    int all_zero = 1;
    for (int i = 0; i < 16; i++) {
        if (result.u8[i] != 0) { all_zero = 0; break; }
    }
    TEST_ASSERT(all_zero, "aesimc(0): should be all zeros");
}

/* Test AESIMC applied twice is not identity (it's a linear transformation) */
static void test_aesimc_not_involution(void) {
    xmm_t input = { .u8 = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                            0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm1\n\t"
        "aesimc %%xmm1, %%xmm2\n\t"
        "movdqu %%xmm2, %0"
        : "=m"(result)
        : "m"(input)
        : "xmm0", "xmm1", "xmm2"
    );

    /* InvMixColumns applied twice is NOT the identity */
    TEST_ASSERT(memcmp(&result, &input, 16) != 0,
                "aesimc applied twice is not identity");
}

/* Test AESIMC linearity: AESIMC(A XOR B) = AESIMC(A) XOR AESIMC(B)
 * InvMixColumns is a linear operation over GF(2^8) */
static void test_aesimc_linearity(void) {
    xmm_t a = { .u8 = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                        0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10} };
    xmm_t b = { .u8 = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
                        0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00} };
    xmm_t imc_a, imc_b, imc_axorb, imc_a_xor_imc_b;

    /* Compute AESIMC(A), AESIMC(B), AESIMC(A XOR B) */
    __asm__ volatile (
        "movdqu %4, %%xmm0\n\t"
        "movdqu %5, %%xmm1\n\t"
        "aesimc %%xmm0, %%xmm2\n\t"   /* imc_a */
        "aesimc %%xmm1, %%xmm3\n\t"   /* imc_b */
        "pxor %%xmm1, %%xmm0\n\t"     /* a XOR b */
        "aesimc %%xmm0, %%xmm4\n\t"   /* imc(a XOR b) */
        "pxor %%xmm3, %%xmm2\n\t"     /* imc_a XOR imc_b */
        "movdqu %%xmm4, %0\n\t"
        "movdqu %%xmm2, %1\n\t"
        "movdqu %4, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm5\n\t"
        "movdqu %%xmm5, %2\n\t"
        "movdqu %5, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm5\n\t"
        "movdqu %%xmm5, %3"
        : "=m"(imc_axorb), "=m"(imc_a_xor_imc_b), "=m"(imc_a), "=m"(imc_b)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
    );

    TEST_ASSERT(memcmp(&imc_axorb, &imc_a_xor_imc_b, 16) == 0,
                "aesimc linearity: AESIMC(A^B) = AESIMC(A) ^ AESIMC(B)");
}

/* Test AESIMC preserves column independence */
static void test_aesimc_column_independence(void) {
    /* Set only one column to non-zero */
    xmm_t input1 = { .u8 = {0x01,0x02,0x03,0x04, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t input2 = { .u8 = {0,0,0,0, 0x01,0x02,0x03,0x04, 0,0,0,0, 0,0,0,0} };
    xmm_t result1, result2;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result1)
        : "m"(input1)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesimc %%xmm0, %%xmm1\n\t"
        "movdqu %%xmm1, %0"
        : "=m"(result2)
        : "m"(input2)
        : "xmm0", "xmm1"
    );

    /* Column 1 of result1 should be zero, column 0 of result2 should be zero */
    int col1_zero = (result1.u8[4] == 0 && result1.u8[5] == 0 &&
                     result1.u8[6] == 0 && result1.u8[7] == 0);
    int col0_zero = (result2.u8[0] == 0 && result2.u8[1] == 0 &&
                     result2.u8[2] == 0 && result2.u8[3] == 0);

    TEST_ASSERT(col1_zero, "aesimc: column independence (col1 stays zero)");
    TEST_ASSERT(col0_zero, "aesimc: column independence (col0 stays zero)");
}

int main(void) {
    TEST_START("AESIMC instruction");
    test_aesimc_reg();
    test_aesimc_mem();
    test_aesimc_zero();
    test_aesimc_not_involution();
    test_aesimc_linearity();
    test_aesimc_column_independence();
    TEST_END();
}
