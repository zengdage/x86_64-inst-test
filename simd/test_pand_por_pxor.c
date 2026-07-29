/*
 * test_pand_por_pxor.c - Test PAND/PANDN/POR/PXOR instructions
 *
 * PAND:  Bitwise AND of 128-bit values.
 * PANDN: Bitwise AND NOT (NOT dest AND src).
 * POR:   Bitwise OR of 128-bit values.
 * PXOR:  Bitwise XOR of 128-bit values.
 *
 * Compile: gcc -o test_pand_por_pxor simd/test_pand_por_pxor.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pand_basic(void) {
    xmm_t a = { .u64 = { 0xFF00FF00FF00FF00ULL, 0xAAAAAAAAAAAAAAAAULL } };
    xmm_t b = { .u64 = { 0x0F0F0F0F0F0F0F0FULL, 0x5555555555555555ULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pand %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == (0xFF00FF00FF00FF00ULL & 0x0F0F0F0F0F0F0F0FULL),
        "pand [0]: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == (0xAAAAAAAAAAAAAAAAULL & 0x5555555555555555ULL),
        "pand [1]: got 0x%016lx", dst.u64[1]);
}

static void test_pand_all_ones(void) {
    xmm_t a = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    xmm_t b = { .u64 = { 0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pand %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == b.u64[0] && dst.u64[1] == b.u64[1],
        "pand with all ones: identity");
}

static void test_pandn_basic(void) {
    xmm_t a = { .u64 = { 0xFF00FF00FF00FF00ULL, 0xAAAAAAAAAAAAAAAAULL } };
    xmm_t b = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pandn %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* PANDN: NOT(a) AND b */
    TEST_ASSERT(dst.u64[0] == (~0xFF00FF00FF00FF00ULL & 0xFFFFFFFFFFFFFFFFULL),
        "pandn [0]: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == (~0xAAAAAAAAAAAAAAAAULL & 0xFFFFFFFFFFFFFFFFULL),
        "pandn [1]: got 0x%016lx", dst.u64[1]);
}

static void test_por_basic(void) {
    xmm_t a = { .u64 = { 0xFF00000000000000ULL, 0x00000000FFFFFFFFULL } };
    xmm_t b = { .u64 = { 0x00FF000000000000ULL, 0xFFFFFFFF00000000ULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "por %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFF000000000000ULL,
        "por [0]: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0xFFFFFFFFFFFFFFFFULL,
        "por [1]: got 0x%016lx", dst.u64[1]);
}

static void test_por_with_zero(void) {
    xmm_t a = { .u64 = { 0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL } };
    xmm_t b = { .u64 = { 0, 0 } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "por %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == a.u64[0] && dst.u64[1] == a.u64[1],
        "por with zero: identity");
}

static void test_pxor_basic(void) {
    xmm_t a = { .u64 = { 0xFF00FF00FF00FF00ULL, 0xAAAAAAAAAAAAAAAAULL } };
    xmm_t b = { .u64 = { 0x0F0F0F0F0F0F0F0FULL, 0x5555555555555555ULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pxor %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == (a.u64[0] ^ b.u64[0]),
        "pxor [0]: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == (a.u64[1] ^ b.u64[1]),
        "pxor [1]: got 0x%016lx", dst.u64[1]);
}

static void test_pxor_self(void) {
    xmm_t a = { .u64 = { 0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pxor %%xmm0, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0,
        "pxor self zeroes: got 0x%016lx_%016lx", dst.u64[1], dst.u64[0]);
}

static void test_pxor_all_ones(void) {
    xmm_t a = { .u64 = { 0xDEADBEEFCAFEBABEULL, 0x1234567890ABCDEFULL } };
    xmm_t b = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pxor %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == ~a.u64[0] && dst.u64[1] == ~a.u64[1],
        "pxor with all ones: NOT");
}

int main(void) {
    TEST_START("PAND/PANDN/POR/PXOR instructions");
    test_pand_basic();
    test_pand_all_ones();
    test_pandn_basic();
    test_por_basic();
    test_por_with_zero();
    test_pxor_basic();
    test_pxor_self();
    test_pxor_all_ones();
    TEST_END();
}
