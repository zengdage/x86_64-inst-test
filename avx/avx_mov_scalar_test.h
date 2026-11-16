#ifndef X86_64_AVX_MOV_SCALAR_TEST_H
#define X86_64_AVX_MOV_SCALAR_TEST_H

#include "../common.h"

#if TEST_BITS == 32
#define MOV_FIELD u32
#define MOV_XMM_LANES 4
#else
#define MOV_FIELD u64
#define MOV_XMM_LANES 2
#endif

int main(void) {
    xmm_t first = {0}, second = {0}, out;
    ymm_t full;
    TEST_START(TEST_NAME);
    for (int i = 0; i < MOV_XMM_LANES; i++) {
        first.MOV_FIELD[i] = (__typeof__(first.MOV_FIELD[i]))(0x11110000U + (unsigned)i);
        second.MOV_FIELD[i] = (__typeof__(second.MOV_FIELD[i]))(0xeeee0000U + (unsigned)i);
    }
    __asm__ volatile("movdqu %1, %%xmm0\n\tmovdqu %2, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(full) : "m"(first), "m"(second) : "xmm0", "xmm1", "ymm2");
    TEST_ASSERT(full.MOV_FIELD[0] == second.MOV_FIELD[0], TEST_NAME " register low scalar");
    for (int i = 1; i < MOV_XMM_LANES; i++)
        TEST_ASSERT(full.MOV_FIELD[i] == first.MOV_FIELD[i], TEST_NAME " first-source lane %d", i);
    for (int i = MOV_XMM_LANES; i < MOV_XMM_LANES * 2; i++)
        TEST_ASSERT(full.MOV_FIELD[i] == 0, TEST_NAME " upper YMM lane %d", i);
    __asm__ volatile(TEST_INSN " %1, %%xmm0\n\t" TEST_INSN " %%xmm0, %0"
                     : "=m"(out.MOV_FIELD[0]) : "m"(second.MOV_FIELD[0]) : "xmm0", "memory");
    TEST_ASSERT(out.MOV_FIELD[0] == second.MOV_FIELD[0], TEST_NAME " memory load/store");
    TEST_END();
}

#endif
