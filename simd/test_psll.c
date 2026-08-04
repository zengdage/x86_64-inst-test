/*
 * test_psll.c - Test PSLLW/PSLLD/PSLLQ instructions
 *
 * PSLLW: Packed shift left logical words.
 * PSLLD: Packed shift left logical doublewords.
 * PSLLQ: Packed shift left logical quadwords.
 * Shift count from imm8 or xmm/mem (low 64 bits).
 * If count > element size, result is 0.
 *
 * Compile: gcc -o test_psll simd/test_psll.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_psllw_imm(void) {
    xmm_t a = { .u16 = {1, 0x00FF, 0x8000, 0xFFFF, 0x1234, 0, 1, 0x7FFF} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psllw $4, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 16, "psllw 1<<4=16: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0x0FF0, "psllw 0xFF<<4=0xFF0: got 0x%04x", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0x0000, "psllw 0x8000<<4=0: got 0x%04x", dst.u16[2]);
    TEST_ASSERT(dst.u16[5] == 0, "psllw 0<<4=0: got %u", dst.u16[5]);
}

static void test_psllw_count_exceeds(void) {
    xmm_t a = { .u16 = {0xFFFF, 0xFFFF, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psllw $16, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "psllw shift 16 (>=16) zeroes: got %u", dst.u16[0]);
}

static void test_pslld_imm(void) {
    xmm_t a = { .u32 = {1, 0x80000000, 0x12345678, 0xFFFFFFFF} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pslld $8, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 256, "pslld 1<<8=256: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0, "pslld 0x80000000<<8=0: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0x34567800, "pslld 0x12345678<<8: got 0x%08x", dst.u32[2]);
}

static void test_psllq_imm(void) {
    xmm_t a = { .u64 = {1, 0x8000000000000000ULL} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psllq $1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 2, "psllq 1<<1=2: got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "psllq MSB<<1=0: got 0x%016lx", dst.u64[1]);
}

static void test_psllq_xmm_count(void) {
    xmm_t a = { .u64 = {0xFF, 0xFF00} };
    xmm_t count = { .u64 = {8, 0} };  /* count from low 64 bits */
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "psllq %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(count) : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.u64[0] == 0xFF00, "psllq xmm count 0xFF<<8: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0xFF0000, "psllq xmm count 0xFF00<<8: got 0x%016lx", dst.u64[1]);
}

static void test_psllw_zero_shift(void) {
    xmm_t a = { .u16 = {0x1234, 0x5678, 0x9ABC, 0xDEF0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psllw $0, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0x1234, "psllw shift 0: unchanged, got 0x%04x", dst.u16[0]);
}

static void test_psll_boundaries(void) {
    xmm_t a = { .u64 = {UINT64_C(1), UINT64_MAX} };
    xmm_t dst;
    xmm_t count;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pslld $31, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == UINT32_C(0x80000000), "pslld last valid count 31");

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pslld $32, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0, "pslld count 32 zeroes all lanes");

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psllq $63, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == UINT64_C(0x8000000000000000),
                "psllq last valid count 63");

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psllq $0xff, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0, "psllq max imm8 zeroes all lanes");

    /* Only the low qword of a vector count operand is consumed. */
    count.u64[0] = 1;
    count.u64[1] = UINT64_MAX;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psllq %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(count) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 2 && dst.u64[1] == UINT64_MAX - 1,
                "psllq vector count ignores upper qword");
}

int main(void) {
    TEST_START("PSLLW/PSLLD/PSLLQ instructions");
    test_psllw_imm();
    test_psllw_count_exceeds();
    test_pslld_imm();
    test_psllq_imm();
    test_psllq_xmm_count();
    test_psllw_zero_shift();
    test_psll_boundaries();
    TEST_END();
}
