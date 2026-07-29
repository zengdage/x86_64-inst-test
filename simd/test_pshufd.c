/*
 * test_pshufd.c - Test PSHUFD/PSHUFHW/PSHUFLW instructions
 *
 * PSHUFD:  Shuffle 32-bit elements using imm8 control. Each 2-bit field selects source dword.
 * PSHUFHW: Shuffle high 4 words (words 4-7), low 4 words unchanged.
 * PSHUFLW: Shuffle low 4 words (words 0-3), high 4 words unchanged.
 *
 * Compile: gcc -o test_pshufd simd/test_pshufd.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pshufd_identity(void) {
    xmm_t src = { .u32 = {10, 20, 30, 40} };
    xmm_t dst;

    /* imm8 = 0xE4 = 11_10_01_00 => identity */
    __asm__ volatile (
        "pshufd $0xE4, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.u32[i] == src.u32[i],
            "pshufd identity [%d]: expected %u, got %u", i, src.u32[i], dst.u32[i]);
    }
}

static void test_pshufd_reverse(void) {
    xmm_t src = { .u32 = {10, 20, 30, 40} };
    xmm_t dst;

    /* imm8 = 0x1B = 00_01_10_11 => reverse */
    __asm__ volatile (
        "pshufd $0x1B, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 40, "pshufd reverse [0]: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 30, "pshufd reverse [1]: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 20, "pshufd reverse [2]: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 10, "pshufd reverse [3]: got %u", dst.u32[3]);
}

static void test_pshufd_broadcast(void) {
    xmm_t src = { .u32 = {42, 0, 0, 0} };
    xmm_t dst;

    /* imm8 = 0x00 = 00_00_00_00 => broadcast element 0 */
    __asm__ volatile (
        "pshufd $0x00, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.u32[i] == 42,
            "pshufd broadcast [%d]: expected 42, got %u", i, dst.u32[i]);
    }
}

static void test_pshufd_swap_pairs(void) {
    xmm_t src = { .u32 = {1, 2, 3, 4} };
    xmm_t dst;

    /* imm8 = 0xB1 = 10_11_00_01 => swap within pairs */
    __asm__ volatile (
        "pshufd $0xB1, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 2, "pshufd swap [0]: got %u", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 1, "pshufd swap [1]: got %u", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 4, "pshufd swap [2]: got %u", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 3, "pshufd swap [3]: got %u", dst.u32[3]);
}

static void test_pshufhw_basic(void) {
    xmm_t src = { .u16 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    /* imm8 = 0x1B = 00_01_10_11 => reverse high words */
    __asm__ volatile (
        "pshufhw $0x1B, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    /* Low words unchanged */
    TEST_ASSERT(dst.u16[0] == 10 && dst.u16[1] == 20 && dst.u16[2] == 30 && dst.u16[3] == 40,
        "pshufhw: low words unchanged");
    /* High words reversed */
    TEST_ASSERT(dst.u16[4] == 80, "pshufhw high [4]: got %u", dst.u16[4]);
    TEST_ASSERT(dst.u16[5] == 70, "pshufhw high [5]: got %u", dst.u16[5]);
    TEST_ASSERT(dst.u16[6] == 60, "pshufhw high [6]: got %u", dst.u16[6]);
    TEST_ASSERT(dst.u16[7] == 50, "pshufhw high [7]: got %u", dst.u16[7]);
}

static void test_pshuflw_basic(void) {
    xmm_t src = { .u16 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    /* imm8 = 0x1B = 00_01_10_11 => reverse low words */
    __asm__ volatile (
        "pshuflw $0x1B, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    /* Low words reversed */
    TEST_ASSERT(dst.u16[0] == 40, "pshuflw low [0]: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 30, "pshuflw low [1]: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 20, "pshuflw low [2]: got %u", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 10, "pshuflw low [3]: got %u", dst.u16[3]);
    /* High words unchanged */
    TEST_ASSERT(dst.u16[4] == 50 && dst.u16[5] == 60 && dst.u16[6] == 70 && dst.u16[7] == 80,
        "pshuflw: high words unchanged");
}

static void test_pshuflw_broadcast(void) {
    xmm_t src = { .u16 = {99, 0, 0, 0, 50, 60, 70, 80} };
    xmm_t dst;

    /* imm8 = 0x00 => broadcast word 0 to all low words */
    __asm__ volatile (
        "pshuflw $0x00, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(src) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.u16[i] == 99,
            "pshuflw broadcast [%d]: expected 99, got %u", i, dst.u16[i]);
    }
}

int main(void) {
    TEST_START("PSHUFD/PSHUFHW/PSHUFLW instructions");
    test_pshufd_identity();
    test_pshufd_reverse();
    test_pshufd_broadcast();
    test_pshufd_swap_pairs();
    test_pshufhw_basic();
    test_pshuflw_basic();
    test_pshuflw_broadcast();
    TEST_END();
}
