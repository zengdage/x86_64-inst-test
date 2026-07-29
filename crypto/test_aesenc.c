/*
 * test_aesenc.c - Test x86-64 AESENC and AESENCLAST instructions
 *
 * AESENC performs one round of AES encryption:
 *   tmp = ShiftRows(state), tmp = SubBytes(tmp),
 *   tmp = MixColumns(tmp), dst = tmp XOR RoundKey
 *
 * AESENCLAST performs the last round (no MixColumns):
 *   tmp = ShiftRows(state), tmp = SubBytes(tmp),
 *   dst = tmp XOR RoundKey
 *
 * Both operate on 128-bit XMM registers.
 * Requires AES-NI support.
 *
 * Compile: gcc -o test_aesenc crypto/test_aesenc.c -O0 -maes
 * Note: Do not use static linking.
 */
#include "../common.h"

/* Test AESENC xmm, xmm */
static void test_aesenc_reg(void) {
    /* Known test vector from NIST AES (one round):
     * State:  00112233445566778899aabbccddeeff
     * RoundKey: 000102030405060708090a0b0c0d0e0f
     */
    xmm_t state = { .u8 = {0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,
                            0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00} };
    xmm_t rkey  = { .u8 = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,
                            0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesenc %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );

    /* Verify determinism: same inputs produce same output */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesenc %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "aesenc xmm,xmm: deterministic");

    /* Output should differ from input */
    TEST_ASSERT(memcmp(&result, &state, 16) != 0,
                "aesenc xmm,xmm: output differs from input state");

    /* AESENC with zero state and zero key should produce known SubBytes(ShiftRows(0)) mixed */
    xmm_t zero_result;
    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "aesenc %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(zero_result)
        :
        : "xmm0", "xmm1"
    );
    /* SubBytes(0x00) = 0x63, after ShiftRows of all-same, MixColumns, XOR 0 key
     * All bytes are 0x63, MixColumns of column [0x63,0x63,0x63,0x63]:
     * result = 2*63 ^ 3*63 ^ 63 ^ 63 = 63*(2^3^1^1) = 63*1 = 63
     * Wait: in GF(2^8), 2*0x63=0xc6, 3*0x63=0xa5
     * col = 0xc6^0xa5^0x63^0x63 = 0xc6^0xa5^0x00 = 0x63
     * So all bytes should be 0x63 */
    int all_63 = 1;
    for (int i = 0; i < 16; i++) {
        if (zero_result.u8[i] != 0x63) { all_63 = 0; break; }
    }
    TEST_ASSERT(all_63, "aesenc(0,0): all bytes should be 0x63");
}

/* Test AESENC xmm, mem */
static void test_aesenc_mem(void) {
    xmm_t state = { .u8 = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
                            0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10} };
    xmm_t rkey  = { .u8 = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
                            0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00} };
    xmm_t result_reg, result_mem;

    /* reg-reg */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesenc %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );

    /* reg-mem */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesenc %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(state), "m"(rkey)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "aesenc: reg-reg and reg-mem produce same result");
}

/* Test AESENCLAST xmm, xmm */
static void test_aesenclast_reg(void) {
    xmm_t state = { .u8 = {0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,
                            0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00} };
    xmm_t rkey  = { .u8 = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,
                            0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesenclast %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &state, 16) != 0,
                "aesenclast xmm,xmm: output differs from input");

    /* AESENCLAST with zero: SubBytes(ShiftRows(0)) ^ 0 = all 0x63 */
    xmm_t zero_result;
    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "aesenclast %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(zero_result)
        :
        : "xmm0", "xmm1"
    );
    int all_63 = 1;
    for (int i = 0; i < 16; i++) {
        if (zero_result.u8[i] != 0x63) { all_63 = 0; break; }
    }
    TEST_ASSERT(all_63, "aesenclast(0,0): all bytes should be 0x63 (no MixColumns)");
}

/* Test AESENCLAST xmm, mem */
static void test_aesenclast_mem(void) {
    xmm_t state = { .u8 = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                            0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10} };
    xmm_t rkey  = { .u8 = {0xff,0xfe,0xfd,0xfc,0xfb,0xfa,0xf9,0xf8,
                            0xf7,0xf6,0xf5,0xf4,0xf3,0xf2,0xf1,0xf0} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesenclast %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesenclast %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(state), "m"(rkey)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "aesenclast: reg-reg and reg-mem produce same result");
}

/* Test that AESENC and AESENCLAST produce different results (MixColumns difference) */
static void test_aesenc_vs_aesenclast(void) {
    xmm_t state = { .u8 = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,
                            0x0f,0xed,0xcb,0xa9,0x87,0x65,0x43,0x21} };
    xmm_t rkey  = { .u8 = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                            0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f} };
    xmm_t enc_result, enclast_result;

    __asm__ volatile (
        "movdqu %2, %%xmm0\n\t"
        "movdqu %3, %%xmm1\n\t"
        "movdqa %%xmm0, %%xmm2\n\t"
        "aesenc %%xmm1, %%xmm0\n\t"
        "aesenclast %%xmm1, %%xmm2\n\t"
        "movdqu %%xmm0, %0\n\t"
        "movdqu %%xmm2, %1"
        : "=m"(enc_result), "=m"(enclast_result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(memcmp(&enc_result, &enclast_result, 16) != 0,
                "aesenc vs aesenclast: different results (MixColumns)");
}

int main(void) {
    TEST_START("AESENC/AESENCLAST instructions");
    test_aesenc_reg();
    test_aesenc_mem();
    test_aesenclast_reg();
    test_aesenclast_mem();
    test_aesenc_vs_aesenclast();
    TEST_END();
}
