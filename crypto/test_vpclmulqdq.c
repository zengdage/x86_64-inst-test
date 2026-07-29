/*
 * test_vpclmulqdq.c - Test x86-64 VEX-encoded VPCLMULQDQ instruction
 *
 * VPCLMULQDQ xmm1, xmm2, xmm3/m128, imm8
 *   Three-operand VEX form of PCLMULQDQ.
 *   Performs carry-less multiplication of two 64-bit values
 *   selected from two 128-bit operands, producing a 128-bit result.
 *
 * The VEX form is non-destructive (separate destination register).
 * Requires PCLMUL + AVX support.
 *
 * Compile: gcc -o test_vpclmulqdq crypto/test_vpclmulqdq.c -O0 -mpclmul -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

/* Test VPCLMULQDQ xmm, xmm, xmm, imm8 */
static void test_vpclmulqdq_reg(void) {
    xmm_t a = { .u64 = {0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL} };
    xmm_t b = { .u64 = {0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL} };
    xmm_t result;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2"
    );

    /* Determinism */
    xmm_t result2;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result2)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "vpclmulqdq xmm,xmm,xmm: deterministic");
}

/* Test VPCLMULQDQ matches legacy PCLMULQDQ */
static void test_vpclmulqdq_matches_legacy(void) {
    xmm_t a = { .u64 = {0xDEADBEEFCAFEBABEULL, 0x0123456789ABCDEFULL} };
    xmm_t b = { .u64 = {0xFEDCBA9876543210ULL, 0xAAAABBBBCCCCDDDDULL} };

    /* Test all four selector values */
    uint8_t imm8_values[] = {0x00, 0x01, 0x10, 0x11};
    for (int i = 0; i < 4; i++) {
        xmm_t result_vex, result_legacy;

        /* VEX form - use separate asm blocks for different imm8 */
        switch (imm8_values[i]) {
        case 0x00:
            __asm__ volatile (
                "vmovdqu %1, %%xmm0\n\t"
                "vmovdqu %2, %%xmm1\n\t"
                "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
                "vmovdqu %%xmm2, %0"
                : "=m"(result_vex) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2");
            __asm__ volatile (
                "movdqu %1, %%xmm0\n\t"
                "movdqu %2, %%xmm1\n\t"
                "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
                "movdqu %%xmm0, %0"
                : "=m"(result_legacy) : "m"(a), "m"(b) : "xmm0", "xmm1");
            break;
        case 0x01:
            __asm__ volatile (
                "vmovdqu %1, %%xmm0\n\t"
                "vmovdqu %2, %%xmm1\n\t"
                "vpclmulqdq $0x01, %%xmm1, %%xmm0, %%xmm2\n\t"
                "vmovdqu %%xmm2, %0"
                : "=m"(result_vex) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2");
            __asm__ volatile (
                "movdqu %1, %%xmm0\n\t"
                "movdqu %2, %%xmm1\n\t"
                "pclmulqdq $0x01, %%xmm1, %%xmm0\n\t"
                "movdqu %%xmm0, %0"
                : "=m"(result_legacy) : "m"(a), "m"(b) : "xmm0", "xmm1");
            break;
        case 0x10:
            __asm__ volatile (
                "vmovdqu %1, %%xmm0\n\t"
                "vmovdqu %2, %%xmm1\n\t"
                "vpclmulqdq $0x10, %%xmm1, %%xmm0, %%xmm2\n\t"
                "vmovdqu %%xmm2, %0"
                : "=m"(result_vex) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2");
            __asm__ volatile (
                "movdqu %1, %%xmm0\n\t"
                "movdqu %2, %%xmm1\n\t"
                "pclmulqdq $0x10, %%xmm1, %%xmm0\n\t"
                "movdqu %%xmm0, %0"
                : "=m"(result_legacy) : "m"(a), "m"(b) : "xmm0", "xmm1");
            break;
        case 0x11:
            __asm__ volatile (
                "vmovdqu %1, %%xmm0\n\t"
                "vmovdqu %2, %%xmm1\n\t"
                "vpclmulqdq $0x11, %%xmm1, %%xmm0, %%xmm2\n\t"
                "vmovdqu %%xmm2, %0"
                : "=m"(result_vex) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2");
            __asm__ volatile (
                "movdqu %1, %%xmm0\n\t"
                "movdqu %2, %%xmm1\n\t"
                "pclmulqdq $0x11, %%xmm1, %%xmm0\n\t"
                "movdqu %%xmm0, %0"
                : "=m"(result_legacy) : "m"(a), "m"(b) : "xmm0", "xmm1");
            break;
        }

        TEST_ASSERT(memcmp(&result_vex, &result_legacy, 16) == 0,
                    "vpclmulqdq matches pclmulqdq with imm8=0x%02x",
                    imm8_values[i]);
    }
}

/* Test VPCLMULQDQ xmm, xmm, mem */
static void test_vpclmulqdq_mem(void) {
    xmm_t a = { .u64 = {0xABCDEF0123456789ULL, 0} };
    xmm_t b = { .u64 = {0xFEDCBA9876543210ULL, 0} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_reg)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2"
    );

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpclmulqdq $0x00, %2, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result_mem)
        : "m"(a), "m"(b)
        : "xmm0", "xmm2"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "vpclmulqdq: reg-reg-reg and reg-reg-mem produce same result");
}

/* Test VPCLMULQDQ non-destructive */
static void test_vpclmulqdq_nondestructive(void) {
    xmm_t a = { .u64 = {0x1111111111111111ULL, 0x2222222222222222ULL} };
    xmm_t b = { .u64 = {0x3333333333333333ULL, 0x4444444444444444ULL} };
    xmm_t a_after, b_after, result;

    __asm__ volatile (
        "vmovdqu %3, %%xmm0\n\t"
        "vmovdqu %4, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm0, %0\n\t"
        "vmovdqu %%xmm1, %1\n\t"
        "vmovdqu %%xmm2, %2"
        : "=m"(a_after), "=m"(b_after), "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(memcmp(&a_after, &a, 16) == 0,
                "vpclmulqdq: source register a preserved");
    TEST_ASSERT(memcmp(&b_after, &b, 16) == 0,
                "vpclmulqdq: source register b preserved");
}

/* Test VPCLMULQDQ with known values */
static void test_vpclmulqdq_known(void) {
    /* clmul(0x03, 0x05) = 0x0F */
    xmm_t a = { .u64 = {0x03, 0} };
    xmm_t b = { .u64 = {0x05, 0} };
    xmm_t result;

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(result.u64[0] == 0x0F,
                "vpclmulqdq(0x03, 0x05): expected 0x0F, got 0x%016" PRIx64,
                result.u64[0]);

    /* x * 1 = x */
    xmm_t val = { .u64 = {0xDEADBEEFCAFEBABEULL, 0} };
    xmm_t one = { .u64 = {1, 0} };

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result)
        : "m"(val), "m"(one)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(result.u64[0] == 0xDEADBEEFCAFEBABEULL && result.u64[1] == 0,
                "vpclmulqdq(x, 1) = x");

    /* x * 0 = 0 */
    xmm_t zero = { .u64 = {0, 0} };

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(result)
        : "m"(val), "m"(zero)
        : "xmm0", "xmm1", "xmm2"
    );

    TEST_ASSERT(result.u64[0] == 0 && result.u64[1] == 0,
                "vpclmulqdq(x, 0) = 0");
}

/* Test different selectors */
static void test_vpclmulqdq_selectors(void) {
    xmm_t a = { .u64 = {0x1111111111111111ULL, 0x2222222222222222ULL} };
    xmm_t b = { .u64 = {0x3333333333333333ULL, 0x4444444444444444ULL} };
    xmm_t r00, r01, r10, r11;

    __asm__ volatile (
        "vmovdqu %4, %%xmm0\n\t"
        "vmovdqu %5, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0\n\t"
        "vpclmulqdq $0x01, %%xmm1, %%xmm0, %%xmm3\n\t"
        "vmovdqu %%xmm3, %1\n\t"
        "vpclmulqdq $0x10, %%xmm1, %%xmm0, %%xmm4\n\t"
        "vmovdqu %%xmm4, %2\n\t"
        "vpclmulqdq $0x11, %%xmm1, %%xmm0, %%xmm5\n\t"
        "vmovdqu %%xmm5, %3"
        : "=m"(r00), "=m"(r01), "=m"(r10), "=m"(r11)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
    );

    TEST_ASSERT(memcmp(&r00, &r01, 16) != 0, "vpclmulqdq: 0x00 != 0x01");
    TEST_ASSERT(memcmp(&r00, &r10, 16) != 0, "vpclmulqdq: 0x00 != 0x10");
    TEST_ASSERT(memcmp(&r00, &r11, 16) != 0, "vpclmulqdq: 0x00 != 0x11");
}

int main(void) {
    TEST_START("VPCLMULQDQ instruction (VEX-encoded)");
    test_vpclmulqdq_reg();
    test_vpclmulqdq_matches_legacy();
    test_vpclmulqdq_mem();
    test_vpclmulqdq_nondestructive();
    test_vpclmulqdq_known();
    test_vpclmulqdq_selectors();
    TEST_END();
}
