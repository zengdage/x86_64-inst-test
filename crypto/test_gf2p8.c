/*
 * test_gf2p8.c - Test x86-64 Galois Field instructions
 *
 * GF2P8AFFINEQB xmm1, xmm2/m128, imm8
 *   Computes an affine transformation in GF(2^8).
 *   For each byte: dst[i] = (matrix_row[i] . src_byte[i]) XOR imm8
 *   where . is the GF(2) dot product (AND + popcount parity).
 *
 * GF2P8AFFINEINVQB xmm1, xmm2/m128, imm8
 *   Same as GF2P8AFFINEQB but first applies GF(2^8) multiplicative inverse
 *   (using the AES polynomial x^8+x^4+x^3+x+1) to each source byte.
 *
 * GF2P8MULB xmm1, xmm2/m128
 *   Performs byte-wise multiplication in GF(2^8) using the AES polynomial.
 *
 * Requires GFNI support.
 *
 * Compile: gcc -o test_gf2p8 crypto/test_gf2p8.c -O0 -mgfni
 * Note: Do not use static linking.
 */
#include "../common.h"

/* Test GF2P8MULB with identity: x * 1 = x */
static void test_gf2p8mulb_identity(void) {
    xmm_t a = { .u8 = {0x01,0x02,0x03,0x53,0x7f,0x80,0xfe,0xff,
                        0x10,0x20,0x30,0x40,0x50,0x60,0x70,0xab} };
    xmm_t one = { .u8 = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8mulb %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(one)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &a, 16) == 0,
                "gf2p8mulb: x * 1 = x");
}

/* Test GF2P8MULB with zero: x * 0 = 0 */
static void test_gf2p8mulb_zero(void) {
    xmm_t a = { .u8 = {0xff,0xfe,0xfd,0xfc,0xfb,0xfa,0xf9,0xf8,
                        0xf7,0xf6,0xf5,0xf4,0xf3,0xf2,0xf1,0xf0} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "gf2p8mulb %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0", "xmm1"
    );

    xmm_t zero = { .u64 = {0, 0} };
    TEST_ASSERT(memcmp(&result, &zero, 16) == 0,
                "gf2p8mulb: x * 0 = 0");
}

/* Test GF2P8MULB commutativity: a * b = b * a */
static void test_gf2p8mulb_commutative(void) {
    xmm_t a = { .u8 = {0x53,0x53,0x53,0x53,0x53,0x53,0x53,0x53,
                        0x53,0x53,0x53,0x53,0x53,0x53,0x53,0x53} };
    xmm_t b = { .u8 = {0xCA,0xCA,0xCA,0xCA,0xCA,0xCA,0xCA,0xCA,
                        0xCA,0xCA,0xCA,0xCA,0xCA,0xCA,0xCA,0xCA} };
    xmm_t result_ab, result_ba;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8mulb %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_ab)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8mulb %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_ba)
        : "m"(b), "m"(a)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result_ab, &result_ba, 16) == 0,
                "gf2p8mulb: commutative (a*b = b*a)");
}

/* Test GF2P8MULB known value: 0x53 * 0xCA in GF(2^8) with AES polynomial */
static void test_gf2p8mulb_known(void) {
    /* In AES GF(2^8), 0x02 * 0x87 should give a known result
     * 0x02 * 0x87: shift left by 1 = 0x10E, reduce mod 0x11B = 0x10E ^ 0x11B = 0x15 */
    xmm_t a = { .u8 = {0x02, 0x02, 0,0,0,0,0,0,0,0,0,0,0,0,0,0} };
    xmm_t b = { .u8 = {0x87, 0x80, 0,0,0,0,0,0,0,0,0,0,0,0,0,0} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8mulb %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    /* 0x02 * 0x87 = 0x15 (after reduction)
     * 0x87 = 10000111b, shift left: 100001110b = 0x10E
     * 0x10E ^ 0x11B = 0x015 */
    TEST_ASSERT(result.u8[0] == 0x15,
                "gf2p8mulb: 0x02*0x87 = 0x%02x (expected 0x15)", result.u8[0]);

    /* 0x02 * 0x80 = 0x1B (xtime of 0x80 hits the polynomial) */
    TEST_ASSERT(result.u8[1] == 0x1B,
                "gf2p8mulb: 0x02*0x80 = 0x%02x (expected 0x1B)", result.u8[1]);
}

/* Test GF2P8MULB xmm, mem */
static void test_gf2p8mulb_mem(void) {
    xmm_t a = { .u8 = {0x53,0xCA,0x01,0xFF,0x80,0x7F,0x00,0x02,
                        0x03,0x09,0x0B,0x0D,0x0E,0xFE,0xAB,0xCD} };
    xmm_t b = { .u8 = {0xCA,0x53,0xFF,0x01,0x02,0x02,0xFF,0x80,
                        0x03,0x0E,0x0D,0x0B,0x09,0xAB,0xFE,0xDC} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8mulb %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "gf2p8mulb %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(a), "m"(b)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "gf2p8mulb: reg-reg and reg-mem produce same result");
}

/* Test GF2P8AFFINEQB with identity matrix and zero constant */
static void test_gf2p8affineqb_identity(void) {
    /* Identity matrix in GF(2) for GFNI:
     * The matrix is stored as 8 bytes per qword, each byte is a row.
     * Row i selects bit i of the output. Each row has one bit set.
     * Row for bit 7 (MSB): 0x80, row for bit 6: 0x40, ..., row for bit 0: 0x01
     * Stored as bytes [0..7] = rows for output bits [7..0] (MSB first) */
    xmm_t data = { .u8 = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                           0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10} };
    xmm_t matrix = { .u8 = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
                             0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8affineqb $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(data), "m"(matrix)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(memcmp(&result, &data, 16) == 0,
                "gf2p8affineqb: identity matrix with imm=0 preserves data");
}

/* Test GF2P8AFFINEQB with zero matrix */
static void test_gf2p8affineqb_zero_matrix(void) {
    xmm_t data = { .u8 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                           0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "gf2p8affineqb $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(data)
        : "xmm0", "xmm1"
    );

    /* Zero matrix * anything = 0, XOR 0 = 0 */
    xmm_t zero = { .u64 = {0, 0} };
    TEST_ASSERT(memcmp(&result, &zero, 16) == 0,
                "gf2p8affineqb: zero matrix gives zero output");
}

/* Test GF2P8AFFINEQB with constant (imm8 XOR) */
static void test_gf2p8affineqb_constant(void) {
    xmm_t data = { .u8 = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "gf2p8affineqb $0x42, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(data)
        : "xmm0", "xmm1"
    );

    /* Zero matrix * zero + 0x42 = 0x42 for each byte */
    int all_42 = 1;
    for (int i = 0; i < 16; i++) {
        if (result.u8[i] != 0x42) { all_42 = 0; break; }
    }
    TEST_ASSERT(all_42, "gf2p8affineqb: zero input with imm=0x42 gives all 0x42");
}

/* Test GF2P8AFFINEQB xmm, mem */
static void test_gf2p8affineqb_mem(void) {
    xmm_t data = { .u8 = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                           0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10} };
    xmm_t matrix = { .u8 = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
                             0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8affineqb $0x55, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(data), "m"(matrix)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "gf2p8affineqb $0x55, %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(data), "m"(matrix)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "gf2p8affineqb: reg-reg and reg-mem produce same result");
}

/* Test GF2P8AFFINEINVQB basic operation */
static void test_gf2p8affineinvqb(void) {
    xmm_t data = { .u8 = {0x01,0x02,0x03,0x53,0xCA,0xFF,0x80,0x7F,
                           0x10,0x20,0x30,0x40,0x50,0x60,0x70,0xAB} };
    /* Identity matrix (row-major, MSB first) */
    xmm_t matrix = { .u8 = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
                             0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8affineinvqb $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(data), "m"(matrix)
        : "xmm0", "xmm1"
    );

    /* With identity matrix and imm=0, result should be GF(2^8) inverse of each byte */
    /* Inverse of 1 is 1 in any field */
    TEST_ASSERT(result.u8[0] == 0x01,
                "gf2p8affineinvqb: inv(0x01) = 0x%02x (expected 0x01)", result.u8[0]);
    /* Inverse of 0 is defined as 0 in AES convention */
    /* Test byte 0 of second qword for another value */

    /* Determinism */
    xmm_t result2;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8affineinvqb $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result2)
        : "m"(data), "m"(matrix)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(memcmp(&result, &result2, 16) == 0,
                "gf2p8affineinvqb: deterministic");
}

/* Test GF2P8AFFINEINVQB: inv(0) = 0 */
static void test_gf2p8affineinvqb_zero(void) {
    xmm_t data = { .u64 = {0, 0} };
    xmm_t matrix = { .u8 = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
                             0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01} };
    xmm_t result;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8affineinvqb $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result)
        : "m"(data), "m"(matrix)
        : "xmm0", "xmm1"
    );

    xmm_t zero = { .u64 = {0, 0} };
    TEST_ASSERT(memcmp(&result, &zero, 16) == 0,
                "gf2p8affineinvqb: inv(0) = 0");
}

/* Test GF2P8AFFINEINVQB xmm, mem */
static void test_gf2p8affineinvqb_mem(void) {
    xmm_t data = { .u8 = {0x53,0xCA,0x01,0xFF,0x80,0x7F,0x00,0x02,
                           0x03,0x09,0x0B,0x0D,0x0E,0xFE,0xAB,0xCD} };
    xmm_t matrix = { .u8 = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
                             0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01} };
    xmm_t result_reg, result_mem;

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8affineinvqb $0x63, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_reg)
        : "m"(data), "m"(matrix)
        : "xmm0", "xmm1"
    );

    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "gf2p8affineinvqb $0x63, %2, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(result_mem)
        : "m"(data), "m"(matrix)
        : "xmm0"
    );

    TEST_ASSERT(memcmp(&result_reg, &result_mem, 16) == 0,
                "gf2p8affineinvqb: reg-reg and reg-mem produce same result");
}

/* Test GF2P8AFFINEINVQB computes GF(2^8) inverse, verified with GF2P8MULB:
 * inv(x) * x = 1 for all non-zero x, inv(0) = 0 */
static void test_gf2p8affineinvqb_inverse_verify(void) {
    xmm_t identity = { .u8 = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
                               0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01} };
    xmm_t data = { .u8 = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                           0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F} };
    xmm_t inverse;

    /* With identity matrix, AFFINEINVQB gives just the GF(2^8) inverse */
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8affineinvqb $0x00, %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(inverse)
        : "m"(data), "m"(identity)
        : "xmm0", "xmm1"
    );

    /* inv(0) = 0, inv(1) = 1 in GF(2^8) */
    TEST_ASSERT(inverse.u8[0] == 0x00,
                "gf2p8affineinvqb: inv(0x00) = 0x%02x (expected 0x00)",
                inverse.u8[0]);
    TEST_ASSERT(inverse.u8[1] == 0x01,
                "gf2p8affineinvqb: inv(0x01) = 0x%02x (expected 0x01)",
                inverse.u8[1]);

    /* Verify: inv(x) * x = 1 for all non-zero x */
    xmm_t product;
    __asm__ volatile (
        "movdqu %1, %%xmm0\n\t"
        "movdqu %2, %%xmm1\n\t"
        "gf2p8mulb %%xmm1, %%xmm0\n\t"
        "movdqu %%xmm0, %0"
        : "=m"(product)
        : "m"(data), "m"(inverse)
        : "xmm0", "xmm1"
    );

    TEST_ASSERT(product.u8[0] == 0x00,
                "gf2p8: 0 * inv(0) = 0x%02x (expected 0x00)", product.u8[0]);
    int all_one = 1;
    for (int i = 1; i < 16; i++) {
        if (product.u8[i] != 0x01) { all_one = 0; break; }
    }
    TEST_ASSERT(all_one,
                "gf2p8: x * inv(x) = 1 for all non-zero x (bytes 1-15)");
}

int main(void) {
    TEST_START("GF2P8 instructions (AFFINEQB/AFFINEINVQB/MULB)");
    test_gf2p8mulb_identity();
    test_gf2p8mulb_zero();
    test_gf2p8mulb_commutative();
    test_gf2p8mulb_known();
    test_gf2p8mulb_mem();
    test_gf2p8affineqb_identity();
    test_gf2p8affineqb_zero_matrix();
    test_gf2p8affineqb_constant();
    test_gf2p8affineqb_mem();
    test_gf2p8affineinvqb();
    test_gf2p8affineinvqb_zero();
    test_gf2p8affineinvqb_mem();
    test_gf2p8affineinvqb_inverse_verify();
    TEST_END();
}
