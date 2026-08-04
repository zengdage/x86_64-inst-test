/*
 * test_aesdec.c - Test x86-64 AESDEC and AESDECLAST instructions
 *
 * AESDEC performs one round of AES decryption:
 *   tmp = InvShiftRows(state), tmp = InvSubBytes(tmp),
 *   tmp = InvMixColumns(tmp), dst = tmp XOR RoundKey
 *
 * AESDECLAST performs the last round of AES decryption (no InvMixColumns):
 *   tmp = InvShiftRows(state), tmp = InvSubBytes(tmp),
 *   dst = tmp XOR RoundKey
 *
 * Both operate on 128-bit XMM registers.
 * Requires AES-NI support.
 *
 * Compile: gcc -o test_aesdec crypto/test_aesdec.c -O0 -maes
 * Note: Do not use static linking.
 */
#include "../common.h"
#include "aes_ref.h"

/* Test AESDEC xmm, xmm */
static void test_aesdec_reg(void) {
    xmm_t state = { .u8 = {0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,
                            0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00} };
    xmm_t rkey  = { .u8 = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,
                            0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00} };
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdec %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 0);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "aesdec xmm,xmm matches independent AES round reference");

    /* Verify determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdec %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "aesdec xmm,xmm: deterministic");

    TEST_ASSERT(memcmp(&result, &state, 16) != 0,
                "aesdec xmm,xmm: output differs from input state");

    /* AESDEC with zero state and zero key:
     * InvSubBytes(0x00) = 0x52, after InvShiftRows of all-same,
     * InvMixColumns, XOR 0 key
     * InvMixColumns of column [0x52,0x52,0x52,0x52]:
     * coeffs are 0x0e,0x0b,0x0d,0x09
     * result = 0x0e*0x52 ^ 0x0b*0x52 ^ 0x0d*0x52 ^ 0x09*0x52
     *        = 0x52 * (0x0e ^ 0x0b ^ 0x0d ^ 0x09) = 0x52 * 0x01 = 0x52 */
    xmm_t zero_result;
    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "aesdec %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(zero_result)
        :
        : "xmm0", "xmm1"
    );
    int all_52 = 1;
    for (int i = 0; i < 16; i++) {
        if (zero_result.u8[i] != 0x52) { all_52 = 0; break; }
    }
    TEST_ASSERT(all_52, "aesdec(0,0): all bytes should be 0x52");
}

/* Test AESDEC xmm, mem */
static void test_aesdec_mem(void) {
    xmm_t state = { .u8 = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
                            0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10} };
    xmm_t rkey  = { .u8 = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
                            0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00} };
    xmm_t result_reg, result_mem;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdec %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 0);

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesdec %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(state), "m"(rkey)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "aesdec: reg-reg and reg-mem produce same result");
    TEST_ASSERT(memcmp(&result_mem, &expected, 16) == 0,
                "aesdec memory source matches independent reference");
}

/* Test AESDECLAST xmm, xmm */
static void test_aesdeclast_reg(void) {
    xmm_t state = { .u8 = {0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,
                            0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00} };
    xmm_t rkey  = { .u8 = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,
                            0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00} };
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdeclast %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 1);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "aesdeclast xmm,xmm matches independent reference");

    TEST_ASSERT(memcmp(&result, &state, 16) != 0,
                "aesdeclast xmm,xmm: output differs from input");

    /* AESDECLAST with zero: InvSubBytes(InvShiftRows(0)) ^ 0 = all 0x52 */
    xmm_t zero_result;
    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "aesdeclast %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(zero_result)
        :
        : "xmm0", "xmm1"
    );
    int all_52 = 1;
    for (int i = 0; i < 16; i++) {
        if (zero_result.u8[i] != 0x52) { all_52 = 0; break; }
    }
    TEST_ASSERT(all_52, "aesdeclast(0,0): all bytes should be 0x52 (no InvMixColumns)");
}

/* Test AESDECLAST xmm, mem */
static void test_aesdeclast_mem(void) {
    xmm_t state = { .u8 = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                            0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10} };
    xmm_t rkey  = { .u8 = {0xff,0xfe,0xfd,0xfc,0xfb,0xfa,0xf9,0xf8,
                            0xf7,0xf6,0xf5,0xf4,0xf3,0xf2,0xf1,0xf0} };
    xmm_t result_reg, result_mem;
    xmm_t expected;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdeclast %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 1);

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "aesdeclast %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(state), "m"(rkey)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "aesdeclast: reg-reg and reg-mem produce same result");
    TEST_ASSERT(memcmp(&result_mem, &expected, 16) == 0,
                "aesdeclast memory source matches independent reference");
}

/* Test that AESDEC and AESDECLAST produce different results */
static void test_aesdec_vs_aesdeclast(void) {
    xmm_t state = { .u8 = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,
                            0x0f,0xed,0xcb,0xa9,0x87,0x65,0x43,0x21} };
    xmm_t rkey  = { .u8 = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                            0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f} };
    xmm_t dec_result, declast_result;

    __asm__ volatile (
        "movdqu %2, %%xmm0\n\t"
        "movdqu %3, %%xmm1\n\t"
        "movdqa %%xmm0, %%xmm2\n\t"
        "aesdec %%xmm1, %%xmm0\n\t"
        "aesdeclast %%xmm1, %%xmm2\n\t"
        "movdqu %%xmm0, %0\n\t"
        "movdqu %%xmm2, %1"
        : "=m"(dec_result), "=m"(declast_result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(memcmp(&dec_result, &declast_result, 16) != 0,
                "aesdec vs aesdeclast: different results (InvMixColumns)");
}

/* Test that AESDEC is not the inverse of AESENC directly
 * (they use different key schedules in practice) */
static void test_aesdec_not_direct_inverse_of_aesenc(void) {
    xmm_t state = { .u8 = {0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
                            0x72,0x6c,0x64,0x21,0x00,0x00,0x00,0x00} };
    xmm_t rkey  = { .u8 = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
                            0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c} };
    xmm_t enc_result, roundtrip;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesenc %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(enc_result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );

    /* AESDEC with same key won't recover original (need AESIMC'd key) */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdec %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(roundtrip)
        : "m"(enc_result), "m"(rkey)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&roundtrip, &state, 16) != 0,
                "aesdec with same key is not direct inverse of aesenc");
}

int main(void) {
    TEST_START("AESDEC/AESDECLAST instructions");
    test_aesdec_reg();
    test_aesdec_mem();
    test_aesdeclast_reg();
    test_aesdeclast_mem();
    test_aesdec_vs_aesdeclast();
    test_aesdec_not_direct_inverse_of_aesenc();
    TEST_END();
}
