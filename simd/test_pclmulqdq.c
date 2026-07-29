/*
 * test_pclmulqdq.c - Test PCLMULQDQ instruction
 *
 * PCLMULQDQ: Carry-less (polynomial) multiplication of two 64-bit values
 * selected from xmm operands by imm8, producing a 128-bit result.
 * imm8 bit 0 selects qword from dest, bit 4 selects from src.
 *
 * Compile: gcc -o test_pclmulqdq simd/test_pclmulqdq.c -O0 -mpclmul
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pclmulqdq_basic(void) {
    /* 0x01 * 0x01 = 0x01 (carry-less) */
    xmm_t a = { .u64 = {1, 0} };
    xmm_t b = { .u64 = {1, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pclmulqdq $0x00, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 1 && dst.u64[1] == 0,
        "pclmulqdq 1*1=1: got 0x%016lx_%016lx", dst.u64[1], dst.u64[0]);
}

static void test_pclmulqdq_simple(void) {
    /* In GF(2): 0x03 * 0x03 = (x+1)*(x+1) = x^2+1 = 0x05 */
    xmm_t a = { .u64 = {0x03, 0} };
    xmm_t b = { .u64 = {0x03, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pclmulqdq $0x00, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0x05, "pclmulqdq 0x03*0x03=0x05: got 0x%016lx", dst.u64[0]);
}

static void test_pclmulqdq_zero(void) {
    xmm_t a = { .u64 = {0, 0} };
    xmm_t b = { .u64 = {0xFFFFFFFFFFFFFFFFULL, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pclmulqdq $0x00, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0,
        "pclmulqdq 0*anything=0: got 0x%016lx_%016lx", dst.u64[1], dst.u64[0]);
}

static void test_pclmulqdq_imm_select(void) {
    xmm_t a = { .u64 = {0x01, 0x02} };
    xmm_t b = { .u64 = {0x03, 0x04} };
    xmm_t dst;

    /* imm8=0x11: select a[1] and b[1] => 0x02 * 0x04 = 0x08 (carry-less) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pclmulqdq $0x11, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 0x02 * 0x04 = (x) * (x^2) = x^3 = 0x08 */
    TEST_ASSERT(dst.u64[0] == 0x08,
        "pclmulqdq $11 a[1]*b[1]: expected 0x08, got 0x%016lx", dst.u64[0]);
}

static void test_pclmulqdq_known_poly(void) {
    /* 0x07 * 0x05 = (x^2+x+1)*(x^2+1) = x^4+x^3+x^2+x^2+x+1 = x^4+x^3+x+1 = 0x1B */
    xmm_t a = { .u64 = {0x07, 0} };
    xmm_t b = { .u64 = {0x05, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pclmulqdq $0x00, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0x1B,
        "pclmulqdq 0x07*0x05=0x1B: got 0x%016lx", dst.u64[0]);
}

int main(void) {
    TEST_START("PCLMULQDQ instruction");
    test_pclmulqdq_basic();
    test_pclmulqdq_simple();
    test_pclmulqdq_zero();
    test_pclmulqdq_imm_select();
    test_pclmulqdq_known_poly();
    TEST_END();
}
