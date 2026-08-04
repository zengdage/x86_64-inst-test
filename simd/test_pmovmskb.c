/*
 * test_pmovmskb.c - Test PMOVMSKB instruction
 *
 * PMOVMSKB: Extract the most significant bit of each byte in the xmm register
 * and store the result as a 16-bit mask in a general-purpose register.
 *
 * Compile: gcc -o test_pmovmskb simd/test_pmovmskb.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pmovmskb_all_zero(void) {
    xmm_t src = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    int32_t result;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmovmskb %%xmm0, %0"
        : "=r"(result) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(result == 0, "pmovmskb all <128: expected 0, got 0x%x", result);
}

static void test_pmovmskb_all_set(void) {
    xmm_t src = { .u8 = {0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
                          0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80} };
    int32_t result;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmovmskb %%xmm0, %0"
        : "=r"(result) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(result == 0xFFFF, "pmovmskb all >=0x80: expected 0xFFFF, got 0x%x", result);
}

static void test_pmovmskb_alternating(void) {
    xmm_t src = { .u8 = {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
                          0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00} };
    int32_t result;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmovmskb %%xmm0, %0"
        : "=r"(result) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(result == 0x5555, "pmovmskb alternating: expected 0x5555, got 0x%x", result);
}

static void test_pmovmskb_single_bit(void) {
    xmm_t src = { .u8 = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    src.u8[7] = 0xFF;
    int32_t result;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmovmskb %%xmm0, %0"
        : "=r"(result) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(result == (1 << 7), "pmovmskb bit 7 only: expected 0x%x, got 0x%x", (1 << 7), result);
}

static void test_pmovmskb_boundary(void) {
    xmm_t src = { .u8 = {0x7F, 0x80, 0x7F, 0x80, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    int32_t result;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pmovmskb %%xmm0, %0"
        : "=r"(result) : "m"(src) : "xmm0"
    );
    /* bit0=0(0x7F), bit1=1(0x80), bit2=0(0x7F), bit3=1(0x80) */
    TEST_ASSERT(result == 0x000A, "pmovmskb boundary 0x7F/0x80: expected 0xA, got 0x%x", result);
}

static void test_pmovmskb_endpoint_bits_and_zero_extension(void) {
    xmm_t src = {0};
    uint64_t result;
    src.u8[0] = 0x80;
    src.u8[15] = 0xff;
    __asm__ volatile (
        "movq $-1, %%rax\n\t" "movdqa %1, %%xmm0\n\t"
        "pmovmskb %%xmm0, %%eax\n\t" "movq %%rax, %0"
        : "=r"(result) : "m"(src) : "rax", "xmm0");
    TEST_ASSERT(result == UINT64_C(0x0000000000008001),
                "pmovmskb isolated lowest/highest bits and zero-extends rax: %#" PRIx64,
                result);
}

int main(void) {
    TEST_START("PMOVMSKB instruction");
    test_pmovmskb_all_zero();
    test_pmovmskb_all_set();
    test_pmovmskb_alternating();
    test_pmovmskb_single_bit();
    test_pmovmskb_boundary();
    test_pmovmskb_endpoint_bits_and_zero_extension();
    TEST_END();
}
