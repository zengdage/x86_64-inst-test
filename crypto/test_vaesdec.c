/*
 * test_vaesdec.c - Test x86-64 VEX-encoded VAESDEC/VAESDECLAST instructions
 *
 * VAESDEC xmm1, xmm2, xmm3/m128
 *   Three-operand VEX form of AESDEC: dst = AESDecRound(src1, src2)
 *   Performs one round of AES decryption (InvShiftRows, InvSubBytes,
 *   InvMixColumns, XOR key).
 *
 * VAESDECLAST xmm1, xmm2, xmm3/m128
 *   Three-operand VEX form of AESDECLAST: dst = AESDecLastRound(src1, src2)
 *   Last round (no InvMixColumns).
 *
 * The VEX forms are non-destructive (separate destination register).
 * Requires AES-NI + AVX support.
 *
 * Compile: gcc -o test_vaesdec crypto/test_vaesdec.c -O0 -maes -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"
#include "aes_ref.h"

/* Test VAESDEC xmm, xmm, xmm */
static void test_vaesdec_reg(void) {
    xmm_t state = { .u8 = {0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,
                            0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00} };
    xmm_t rkey  = { .u8 = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,
                            0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00} };
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 0);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "vaesdec matches independent AES round reference");

    /* Determinism */
    xmm_t result2;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result2)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "vaesdec xmm,xmm,xmm: deterministic");

    TEST_ASSERT(memcmp(&result, &state, 16) != 0,
                "vaesdec xmm,xmm,xmm: output differs from input");
}

/* Test VAESDEC matches legacy AESDEC */
static void test_vaesdec_matches_legacy(void) {
    xmm_t state = { .u8 = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
                            0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10} };
    xmm_t rkey  = { .u8 = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
                            0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00} };
    xmm_t result_vex, result_legacy;
    xmm_t expected;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_vex)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 0);

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdec %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_legacy)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result_vex, &result_legacy, 16) == 0,
                "vaesdec matches legacy aesdec");
    TEST_ASSERT(memcmp(&result_vex, &expected, 16) == 0,
                "vaesdec and legacy result match independent reference");
}

/* Test VAESDEC xmm, xmm, mem */
static void test_vaesdec_mem(void) {
    xmm_t state = { .u8 = {0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
                            0x72,0x6c,0x64,0x21,0x00,0x00,0x00,0x00} };
    xmm_t rkey  = { .u8 = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
                            0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c} };
    xmm_t result_reg, result_mem;
    xmm_t expected;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_reg)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 0);

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vaesdec %2, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_mem)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm2"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "vaesdec: reg-reg-reg and reg-reg-mem produce same result");
    TEST_ASSERT(memcmp(&result_mem, &expected, 16) == 0,
                "vaesdec memory source matches independent reference");
}

/* Test VAESDEC non-destructive */
static void test_vaesdec_nondestructive(void) {
    xmm_t state = { .u8 = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                            0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10} };
    xmm_t rkey  = { .u8 = {0xff,0xfe,0xfd,0xfc,0xfb,0xfa,0xf9,0xf8,
                            0xf7,0xf6,0xf5,0xf4,0xf3,0xf2,0xf1,0xf0} };
    xmm_t state_after, rkey_after, result;

    __asm__ volatile (
        "vmovdqu %3, %%xmm0\n\t"
        "vmovdqu %4, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm0, %0\n\t"
        "vmovdqu %%xmm1, %1\n\t"
        "vmovdqu %%xmm2, %2"
        : "=m"(state_after), "=m"(rkey_after), "=m"(result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(memcmp(&state_after, &state, 16) == 0,
                "vaesdec: source state register preserved");
    TEST_ASSERT(memcmp(&rkey_after, &rkey, 16) == 0,
                "vaesdec: source key register preserved");
}

/* Test VAESDECLAST xmm, xmm, xmm */
static void test_vaesdeclast_reg(void) {
    xmm_t state = { .u8 = {0xff,0xee,0xdd,0xcc,0xbb,0xaa,0x99,0x88,
                            0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00} };
    xmm_t rkey  = { .u8 = {0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,
                            0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00} };
    xmm_t result;
    xmm_t expected;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdeclast %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 1);
    TEST_ASSERT(memcmp(&result, &expected, 16) == 0,
                "vaesdeclast matches independent AES last-round reference");

    TEST_ASSERT(memcmp(&result, &state, 16) != 0,
                "vaesdeclast xmm,xmm,xmm: output differs from input");
}

/* Test VAESDECLAST matches legacy AESDECLAST */
static void test_vaesdeclast_matches_legacy(void) {
    xmm_t state = { .u8 = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
                            0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10} };
    xmm_t rkey  = { .u8 = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,
                            0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00} };
    xmm_t result_vex, result_legacy;
    xmm_t expected;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdeclast %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_vex)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 1);

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "aesdeclast %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_legacy)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result_vex, &result_legacy, 16) == 0,
                "vaesdeclast matches legacy aesdeclast");
    TEST_ASSERT(memcmp(&result_vex, &expected, 16) == 0,
                "vaesdeclast and legacy result match independent reference");
}

/* Test VAESDECLAST xmm, xmm, mem */
static void test_vaesdeclast_mem(void) {
    xmm_t state = { .u8 = {0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
                            0x72,0x6c,0x64,0x21,0x00,0x00,0x00,0x00} };
    xmm_t rkey  = { .u8 = {0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
                            0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c} };
    xmm_t result_reg, result_mem;
    xmm_t expected;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdeclast %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_reg)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 1);

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vaesdeclast %2, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_mem)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm2"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "vaesdeclast: reg-reg-reg and reg-reg-mem produce same result");
    TEST_ASSERT(memcmp(&result_mem, &expected, 16) == 0,
                "vaesdeclast memory source matches independent reference");
}

/* Test VAESDEC vs VAESDECLAST difference */
static void test_vaesdec_vs_vaesdeclast(void) {
    xmm_t state = { .u8 = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,
                            0x0f,0xed,0xcb,0xa9,0x87,0x65,0x43,0x21} };
    xmm_t rkey  = { .u8 = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x11,0x22,
                            0x33,0x44,0x55,0x66,0x77,0x88,0x99,0x00} };
    xmm_t dec_result, declast_result;

    __asm__ volatile (
        "vmovdqu %2, %%xmm0\n\t"
        "vmovdqu %3, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vaesdeclast %%xmm1, %%xmm0, %%xmm3\n\t"
        "vmovdqu %%xmm2, %0\n\t"
        "vmovdqu %%xmm3, %1"
        : "=m"(dec_result), "=m"(declast_result)
        : "m"(state), "m"(rkey)
        : "xmm0", "xmm1", "xmm2", "xmm3"
    );

    TEST_ASSERT(memcmp(&dec_result, &declast_result, 16) != 0,
                "vaesdec vs vaesdeclast: different (InvMixColumns)");
}

/* Test VAESDEC with zero */
static void test_vaesdec_zero(void) {
    xmm_t result;

    __asm__ volatile (
        "vpxor %%xmm0, %%xmm0, %%xmm0\n\t"
        "vpxor %%xmm1, %%xmm1, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result)
        :
        : "xmm0", "xmm1", "xmm2"
    );

    int all_52 = 1;
    for (int i = 0; i < 16; i++) {
        if (result.u8[i] != 0x52) { all_52 = 0; break; }
    }
    TEST_ASSERT(all_52, "vaesdec(0,0): all bytes should be 0x52");
}

int main(void) {
    TEST_START("VAESDEC/VAESDECLAST instructions (VEX-encoded)");
    test_vaesdec_reg();
    test_vaesdec_matches_legacy();
    test_vaesdec_mem();
    test_vaesdec_nondestructive();
    test_vaesdeclast_reg();
    test_vaesdeclast_matches_legacy();
    test_vaesdeclast_mem();
    test_vaesdec_vs_vaesdeclast();
    test_vaesdec_zero();
    TEST_END();
}
