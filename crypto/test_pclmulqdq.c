/*
 * test_pclmulqdq.c - Test x86-64 PCLMULQDQ instruction
 *
 * PCLMULQDQ performs carry-less (polynomial) multiplication of two 64-bit
 * values selected from two 128-bit operands, producing a 128-bit result.
 *
 * The imm8 selects which 64-bit halves to multiply:
 *   imm8[0] selects src1 half (0=low, 1=high)
 *   imm8[4] selects src2 half (0=low, 1=high)
 *
 * Common imm8 values:
 *   0x00 - low * low
 *   0x01 - high * low
 *   0x10 - low * high
 *   0x11 - high * high
 *
 * Used in GCM mode of AES, CRC calculations, etc.
 * Requires PCLMUL support.
 *
 * Compile: gcc -o test_pclmulqdq crypto/test_pclmulqdq.c -O0 -mpclmul
 * Note: Do not use static linking.
 */
#include "../common.h"
#include "clmul_ref.h"

/* Test PCLMULQDQ with zero */
static void test_pclmulqdq_zero(void) {
    xmm_t result;

    __asm__ volatile (
        "pxor %%xmm0, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        :
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(result.u64[0] == 0 && result.u64[1] == 0,
                "pclmulqdq(0, 0, 0x00): result is zero");
}

/* Test PCLMULQDQ: x * 1 = x (carry-less multiplication identity) */
static void test_pclmulqdq_identity(void) {
    xmm_t a = { .u64 = {0x123456789ABCDEF0ULL, 0} };
    xmm_t b = { .u64 = {1, 0} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(result.u64[0] == 0x123456789ABCDEF0ULL && result.u64[1] == 0,
                "pclmulqdq(x, 1, 0x00): result = x");
}

/* Test PCLMULQDQ: known values
 * clmul(0x03, 0x05) in binary: (x+1)*(x^2+1) = x^3+x^2+x+1 = 0x0F
 * But with 64-bit operands: 0x03 * 0x05 = 0x0F */
static void test_pclmulqdq_known(void) {
    xmm_t a = { .u64 = {0x03, 0} };
    xmm_t b = { .u64 = {0x05, 0} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(result.u64[0] == 0x0F,
                "pclmulqdq(0x03, 0x05): expected 0x0F, got 0x%016" PRIx64,
                result.u64[0]);

    /* clmul(0x07, 0x03) = (x^2+x+1)*(x+1) = x^3+1 = 0x09
     * Actually: x^3 + x^2 + x + x^2 + x + 1 = x^3 + 1 = 0x09 */
    xmm_t c = { .u64 = {0x07, 0} };
    xmm_t d = { .u64 = {0x03, 0} };
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(c), "m"(d)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(result.u64[0] == 0x09,
                "pclmulqdq(0x07, 0x03): expected 0x09, got 0x%016" PRIx64,
                result.u64[0]);
}

/* Test PCLMULQDQ: commutativity a*b = b*a */
static void test_pclmulqdq_commutative(void) {
    xmm_t a = { .u64 = {0xDEADBEEFCAFEBABEULL, 0} };
    xmm_t b = { .u64 = {0x0123456789ABCDEFULL, 0} };
    xmm_t result_ab, result_ba;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_ab)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_ba)
        : "m"(b), "m"(a)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result_ab, &result_ba, 16) == 0,
                "pclmulqdq: commutative (a*b = b*a)");
}

/* Test different imm8 selectors */
static void test_pclmulqdq_selectors(void) {
    xmm_t a = { .u64 = {0x1111111111111111ULL, 0x2222222222222222ULL} };
    xmm_t b = { .u64 = {0x3333333333333333ULL, 0x4444444444444444ULL} };
    xmm_t r00, r01, r10, r11;

    __asm__ volatile (
        "movdqu %4, %%xmm0\n\t"
        "movdqu %5, %%xmm1\n\t"
        "movdqa %%xmm0, %%xmm2\n\t"
        "movdqa %%xmm0, %%xmm3\n\t"
        "movdqa %%xmm0, %%xmm4\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "pclmulqdq $0x01, %%xmm1, %%xmm2\n\t"
        "pclmulqdq $0x10, %%xmm1, %%xmm3\n\t"
        "pclmulqdq $0x11, %%xmm1, %%xmm4\n\t"
        "movdqu %%xmm0, %0\n\t"
        "movdqu %%xmm2, %1\n\t"
        "movdqu %%xmm3, %2\n\t"
        "movdqu %%xmm4, %3"
        : "=m"(r00), "=m"(r01), "=m"(r10), "=m"(r11)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
    );

    xmm_t results[] = {r00, r01, r10, r11};
    const unsigned ai[] = {0, 1, 0, 1};
    const unsigned bi[] = {0, 0, 1, 1};
    for (int i = 0; i < 4; i++) {
        uint64_t low, high;
        clmul64_reference(a.u64[ai[i]], b.u64[bi[i]], &low, &high);
        TEST_ASSERT(results[i].u64[0] == low && results[i].u64[1] == high,
                    "pclmulqdq selector %d matches scalar reference", i);
    }
}

/* Test PCLMULQDQ xmm, mem */
static void test_pclmulqdq_mem(void) {
    xmm_t a = { .u64 = {0xABCDEF0123456789ULL, 0} };
    xmm_t b = { .u64 = {0xFEDCBA9876543210ULL, 0} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "pclmulqdq $0x00, %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(a), "m"(b)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "pclmulqdq: reg-reg and reg-mem produce same result");
    uint64_t low, high;
    clmul64_reference(a.u64[0], b.u64[0], &low, &high);
    TEST_ASSERT(result_mem.u64[0] == low && result_mem.u64[1] == high,
                "pclmulqdq memory source matches scalar reference");
}

/* Test x * x gives known pattern for carry-less multiply */
static void test_pclmulqdq_self_multiply(void) {
    /* clmul(x, x) spreads bits: bit i goes to bit 2i */
    xmm_t a = { .u64 = {0x0F, 0} }; /* bits 0-3 set */
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqa %%xmm0, %%xmm1\n\t"
        "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0", "xmm1"
    );

    /* 0x0F = 1111b, self-clmul should spread to bits 0,2,4,6 = 0x55 */
    TEST_ASSERT(result.u64[0] == 0x55,
                "pclmulqdq(0x0F, 0x0F): expected 0x55, got 0x%016" PRIx64,
                result.u64[0]);
}

static void test_pclmulqdq_high_immediate_bits(void) {
    xmm_t a = { .u64 = {UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210)} };
    xmm_t b = { .u64 = {UINT64_C(0x1111111111111111), UINT64_C(0x2222222222222222)} };
    xmm_t r11, rff;

    /* Only imm8 bits 0 and 4 select operands; all other bits are ignored. */
    __asm__ volatile (
        "movdqu %2, %%xmm0\n\t"
        "movdqu %3, %%xmm1\n\t"
        "movdqa %%xmm0, %%xmm2\n\t"
        "pclmulqdq $0x11, %%xmm1, %%xmm0\n\t"
        "pclmulqdq $0xff, %%xmm1, %%xmm2\n\t"
        "movdqu %%xmm0, %0\n\t"
        "movdqu %%xmm2, %1"
        : "=m"(r11), "=m"(rff)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(memcmp(&r11, &rff, sizeof(r11)) == 0,
                "pclmulqdq imm=0xff: reserved immediate bits ignored");
}

int main(void) {
    TEST_START("PCLMULQDQ instruction");
    test_pclmulqdq_zero();
    test_pclmulqdq_identity();
    test_pclmulqdq_known();
    test_pclmulqdq_commutative();
    test_pclmulqdq_selectors();
    test_pclmulqdq_mem();
    test_pclmulqdq_self_multiply();
    test_pclmulqdq_high_immediate_bits();
    TEST_END();
}
