/*
 * test_psrl.c - Test PSRLW/PSRLD/PSRLQ instructions
 *
 * PSRLW: Packed shift right logical words (zero-fill).
 * PSRLD: Packed shift right logical doublewords (zero-fill).
 * PSRLQ: Packed shift right logical quadwords (zero-fill).
 * If count > element size, result is 0.
 *
 * Compile: gcc -o test_psrl simd/test_psrl.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_psrlw_imm(void) {
    xmm_t a = { .u16 = {0xFFFF, 0x8000, 0x00FF, 1, 16, 256, 0, 0x1234} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrlw $4, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0x0FFF, "psrlw 0xFFFF>>4=0x0FFF: got 0x%04x", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0x0800, "psrlw 0x8000>>4=0x0800: got 0x%04x", dst.u16[1]);
    TEST_ASSERT(dst.u16[3] == 0, "psrlw 1>>4=0: got %u", dst.u16[3]);
    TEST_ASSERT(dst.u16[4] == 1, "psrlw 16>>4=1: got %u", dst.u16[4]);
}

static void test_psrlw_count_exceeds(void) {
    xmm_t a = { .u16 = {0xFFFF, 0xFFFF, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrlw $16, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "psrlw shift>=16 zeroes: got %u", dst.u16[0]);
}

static void test_psrld_imm(void) {
    xmm_t a = { .u32 = {0xFFFFFFFF, 0x80000000, 256, 1} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrld $8, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0x00FFFFFF, "psrld 0xFFFFFFFF>>8: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0x00800000, "psrld 0x80000000>>8: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 1, "psrld 256>>8=1: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0, "psrld 1>>8=0: got %u", dst.u32[3]);
}

static void test_psrlq_imm(void) {
    xmm_t a = { .u64 = {0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrlq $63, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 1, "psrlq MSB>>63=1: got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 1, "psrlq all_ones>>63=1: got %lu", dst.u64[1]);
}

static void test_psrlq_xmm_count(void) {
    xmm_t a = { .u64 = {0xFF00, 0xFF0000} };
    xmm_t count = { .u64 = {8, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "psrlq %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(count) : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.u64[0] == 0xFF, "psrlq xmm count: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0xFF00, "psrlq xmm count: got 0x%016lx", dst.u64[1]);
}

static void test_psrlw_zero_shift(void) {
    xmm_t a = { .u16 = {0xABCD, 0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psrlw $0, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0xABCD, "psrlw shift 0: unchanged, got 0x%04x", dst.u16[0]);
}

int main(void) {
    TEST_START("PSRLW/PSRLD/PSRLQ instructions");
    test_psrlw_imm();
    test_psrlw_count_exceeds();
    test_psrld_imm();
    test_psrlq_imm();
    test_psrlq_xmm_count();
    test_psrlw_zero_shift();
    TEST_END();
}
