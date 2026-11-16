#ifndef X86_64_AVX_LANE128_TEST_H
#define X86_64_AVX_LANE128_TEST_H

#include "../common.h"

#if TEST_EXTRACT
int main(void) {
    ymm_t src = { .u32 = {0,1,2,3,4,5,6,7} };
    xmm_t low, high, imm_high;
    TEST_START(TEST_NAME);
    __asm__ volatile("vmovdqu %3, %%ymm0\n\t"
                     TEST_INSN " $0, %%ymm0, %0\n\t"
                     TEST_INSN " $1, %%ymm0, %1\n\t"
                     TEST_INSN " $0xff, %%ymm0, %2"
                     : "=m"(low), "=m"(high), "=m"(imm_high)
                     : "m"(src) : "ymm0");
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(low.u32[i] == src.u32[i], TEST_NAME " low lane %d", i);
        TEST_ASSERT(high.u32[i] == src.u32[i + 4], TEST_NAME " high lane %d", i);
        TEST_ASSERT(imm_high.u32[i] == high.u32[i], TEST_NAME " high immediate bits %d", i);
    }
    TEST_END();
}
#else
int main(void) {
    ymm_t a = { .u32 = {0,1,2,3,4,5,6,7} };
    ymm_t b = { .u32 = {10,11,12,13,14,15,16,17} };
    ymm_t out;
    TEST_START(TEST_NAME);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " $0x20, %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(out.u32[i] == a.u32[i], TEST_NAME " a.low lane %d", i);
        TEST_ASSERT(out.u32[i + 4] == b.u32[i], TEST_NAME " b.low lane %d", i);
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " $0x31, %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(out.u32[i] == a.u32[i + 4], TEST_NAME " a.high lane %d", i);
        TEST_ASSERT(out.u32[i + 4] == b.u32[i + 4], TEST_NAME " b.high lane %d", i);
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     TEST_INSN " $0x88, %%ymm0, %%ymm0, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a) : "ymm0", "ymm1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(out.u32[i] == 0, TEST_NAME " zero lane %d", i);
    TEST_END();
}
#endif

#endif
