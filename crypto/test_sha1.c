/*
 * test_sha1.c - Test x86-64 SHA1 instructions
 *
 * SHA1RNDS4 xmm1, xmm2/m128, imm8
 *   Performs four rounds of SHA-1 operation.
 *   imm8 selects the round function (0-3 for rounds 0-19, 20-39, 40-59, 60-79).
 *
 * SHA1MSG1 xmm1, xmm2/m128
 *   Performs an intermediate calculation for the next four SHA-1 message dwords.
 *
 * SHA1MSG2 xmm1, xmm2/m128
 *   Performs the final calculation for the next four SHA-1 message dwords.
 *
 * SHA1NEXTE xmm1, xmm2/m128
 *   Calculates SHA-1 state variable E after four rounds.
 *
 * Requires SHA extension support.
 *
 * Compile: gcc -o test_sha1 crypto/test_sha1.c -O0 -msha
 * Note: Do not use static linking.
 */
#include "../common.h"

/* Test SHA1RNDS4 with different round functions */
static void test_sha1rnds4_rounds(void) {
    /* SHA-1 initial state: H0=67452301, H1=EFCDAB89, H2=98BADCFE, H3=10325476 */
    xmm_t state = { .u32 = {0x10325476, 0x98BADCFE, 0xEFCDAB89, 0x67452301} };
    xmm_t msg   = { .u32 = {0x80000000, 0x00000000, 0x00000000, 0x00000000} };
    xmm_t result0, result1, result2, result3;

    /* Round function 0 (Ch, rounds 0-19) */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1rnds4 $0, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result0)
        : "m"(state), "m"(msg)
        : "xmm0", "xmm1"
    );

    /* Round function 1 (Parity, rounds 20-39) */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1rnds4 $1, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result1)
        : "m"(state), "m"(msg)
        : "xmm0", "xmm1"
    );

    /* Round function 2 (Maj, rounds 40-59) */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1rnds4 $2, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(state), "m"(msg)
        : "xmm0", "xmm1"
    );

    /* Round function 3 (Parity, rounds 60-79) */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1rnds4 $3, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result3)
        : "m"(state), "m"(msg)
        : "xmm0", "xmm1"
    );

    /* Different round functions should produce different results */
    TEST_ASSERT(memcmp(&result0, &result1, 16) != 0,
                "sha1rnds4: round 0 != round 1");
    TEST_ASSERT(memcmp(&result0, &result2, 16) != 0,
                "sha1rnds4: round 0 != round 2");
    /* Round 1 and 3 use the same function (Parity) but different K constants */
    TEST_ASSERT(memcmp(&result1, &result3, 16) != 0,
                "sha1rnds4: round 1 != round 3 (different K)");
    /* Output should differ from input */
    TEST_ASSERT(memcmp(&result0, &state, 16) != 0,
                "sha1rnds4: output differs from input state");
}

/* Test SHA1RNDS4 determinism */
static void test_sha1rnds4_deterministic(void) {
    xmm_t state = { .u32 = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476} };
    xmm_t msg   = { .u32 = {0x61626364, 0x65666768, 0x696A6B6C, 0x6D6E6F70} };
    xmm_t result1, result2;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1rnds4 $0, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result1)
        : "m"(state), "m"(msg)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1rnds4 $0, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(state), "m"(msg)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result1, &result2, 16) == 0,
                "sha1rnds4: deterministic");
}

/* Test SHA1RNDS4 xmm, mem */
static void test_sha1rnds4_mem(void) {
    xmm_t state = { .u32 = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476} };
    xmm_t msg   = { .u32 = {0x80000000, 0, 0, 0} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1rnds4 $0, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(state), "m"(msg)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "sha1rnds4 $0, %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(state), "m"(msg)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "sha1rnds4: reg-reg and reg-mem produce same result");
}

/* Test SHA1MSG1 */
static void test_sha1msg1(void) {
    xmm_t w0_3 = { .u32 = {0x61626364, 0x65666768, 0x696A6B6C, 0x6D6E6F70} };
    xmm_t w4_7 = { .u32 = {0x71727374, 0x75767778, 0x797A3031, 0x32333435} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1msg1 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(w0_3), "m"(w4_7)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &w0_3, 16) != 0,
                "sha1msg1: output differs from input");

    /* Test determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1msg1 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(w0_3), "m"(w4_7)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "sha1msg1: deterministic");
}

/* Test SHA1MSG1 xmm, mem */
static void test_sha1msg1_mem(void) {
    xmm_t w0 = { .u32 = {0x11111111, 0x22222222, 0x33333333, 0x44444444} };
    xmm_t w1 = { .u32 = {0x55555555, 0x66666666, 0x77777777, 0x88888888} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1msg1 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(w0), "m"(w1)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "sha1msg1 %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(w0), "m"(w1)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "sha1msg1: reg-reg and reg-mem produce same result");
}

/* Test SHA1MSG2 */
static void test_sha1msg2(void) {
    xmm_t a = { .u32 = {0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321} };
    xmm_t b = { .u32 = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1msg2 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &a, 16) != 0,
                "sha1msg2: output differs from input");

    /* Determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1msg2 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "sha1msg2: deterministic");
}

/* Test SHA1MSG2 xmm, mem */
static void test_sha1msg2_mem(void) {
    xmm_t a = { .u32 = {0x11111111, 0x22222222, 0x33333333, 0x44444444} };
    xmm_t b = { .u32 = {0x55555555, 0x66666666, 0x77777777, 0x88888888} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1msg2 %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "sha1msg2 %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(a), "m"(b)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "sha1msg2: reg-reg and reg-mem produce same result");
}

/* Test SHA1NEXTE */
static void test_sha1nexte(void) {
    xmm_t a = { .u32 = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476} };
    xmm_t b = { .u32 = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1nexte %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    /* SHA1NEXTE: dst[127:96] = src1[127:96] ROL 30 + src2[127:96]
     * Other dwords come from src2 unchanged */
    TEST_ASSERT(result.u32[0] == b.u32[0], "sha1nexte: dword0 from src2");
    TEST_ASSERT(result.u32[1] == b.u32[1], "sha1nexte: dword1 from src2");
    TEST_ASSERT(result.u32[2] == b.u32[2], "sha1nexte: dword2 from src2");

    /* Determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1nexte %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "sha1nexte: deterministic");
}

/* Test SHA1NEXTE xmm, mem */
static void test_sha1nexte_mem(void) {
    xmm_t a = { .u32 = {0x11111111, 0x22222222, 0x33333333, 0x44444444} };
    xmm_t b = { .u32 = {0x55555555, 0x66666666, 0x77777777, 0x88888888} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "sha1nexte %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "sha1nexte %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(a), "m"(b)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "sha1nexte: reg-reg and reg-mem produce same result");
}

int main(void) {
    TEST_START("SHA1 instructions (SHA1RNDS4/SHA1MSG1/SHA1MSG2/SHA1NEXTE)");
    test_sha1rnds4_rounds();
    test_sha1rnds4_deterministic();
    test_sha1rnds4_mem();
    test_sha1msg1();
    test_sha1msg1_mem();
    test_sha1msg2();
    test_sha1msg2_mem();
    test_sha1nexte();
    test_sha1nexte_mem();
    TEST_END();
}
