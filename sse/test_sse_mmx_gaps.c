/* TARGETS: MASKMOVQ MOVNTQ PSHUFW MOVDQ2Q MOVQ2DQ */
#include "../common.h"

static void test_maskmovq(void) {
    const uint64_t data = UINT64_C(0x8877665544332211);
    const uint64_t mask = UINT64_C(0x8000800080008000);
    uint8_t output[10];
    memset(output, 0xa5, sizeof(output));

    __asm__ volatile("movq %0, %%mm0\n\tmovq %1, %%mm1\n\t"
                     "maskmovq %%mm1, %%mm0\n\temms"
                     : : "m"(data), "m"(mask), "D"(output + 1)
                     : "mm0", "mm1", "memory");
    TEST_ASSERT(output[0] == 0xa5 && output[9] == 0xa5,
                "MASKMOVQ preserves guard bytes");
    for (int i = 0; i < 8; i++) {
        uint8_t expected = (mask & (UINT64_C(0x80) << (8 * i))) ?
                           (uint8_t)(data >> (8 * i)) : 0xa5;
        TEST_ASSERT(output[i + 1] == expected, "MASKMOVQ byte %d", i);
    }
}

static void test_movntq(void) {
    const uint64_t input = UINT64_C(0x0123456789abcdef);
    uint64_t output = 0;
    __asm__ volatile("movq %1, %%mm0\n\tmovntq %%mm0, %0\n\t"
                     "sfence\n\temms"
                     : "=m"(output) : "m"(input) : "mm0", "memory");
    TEST_ASSERT(output == input, "MOVNTQ stores all 64 bits");
}

static void test_pshufw(void) {
    const uint64_t input = UINT64_C(0x4444333322221111);
    uint64_t output;
    __asm__ volatile("movq %1, %%mm0\n\tpshufw $0x1b, %%mm0, %%mm1\n\t"
                     "movq %%mm1, %0\n\temms"
                     : "=m"(output) : "m"(input) : "mm0", "mm1");
    TEST_ASSERT(output == UINT64_C(0x1111222233334444),
                "PSHUFW reverses four words");
}

static void test_mmx_xmm_moves(void) {
    const xmm_t input = { .u64 = {
        UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210)
    } };
    uint64_t mm_output;
    xmm_t xmm_output;

    __asm__ volatile("movdqu %1, %%xmm0\n\tmovdq2q %%xmm0, %%mm0\n\t"
                     "movq %%mm0, %0\n\temms"
                     : "=m"(mm_output) : "m"(input) : "xmm0", "mm0");
    TEST_ASSERT(mm_output == input.u64[0], "MOVDQ2Q copies low XMM qword");

    __asm__ volatile("movq %1, %%mm0\n\tpcmpeqd %%xmm0, %%xmm0\n\t"
                     "movq2dq %%mm0, %%xmm0\n\tmovdqu %%xmm0, %0\n\temms"
                     : "=m"(xmm_output) : "m"(mm_output) : "mm0", "xmm0");
    TEST_ASSERT(xmm_output.u64[0] == mm_output, "MOVQ2DQ copies MMX qword");
    TEST_ASSERT(xmm_output.u64[1] == 0, "MOVQ2DQ clears high XMM qword");
}

int main(void) {
    TEST_START("SSE MMX-form instruction gaps");
    test_maskmovq();
    test_movntq();
    test_pshufw();
    test_mmx_xmm_moves();
    TEST_END();
}
