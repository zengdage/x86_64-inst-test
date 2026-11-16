#ifndef X86_64_AVX_BLEND_IMM_TEST_H
#define X86_64_AVX_BLEND_IMM_TEST_H

#include "../common.h"

#if TEST_BITS == 32
#define BLEND_FIELD u32
#define BLEND_LANES 8
#define BLEND_XMM_LANES 4
#else
#define BLEND_FIELD u64
#define BLEND_LANES 4
#define BLEND_XMM_LANES 2
#endif

int main(void) {
    ymm_t a = {0}, b = {0}, out, initial;
    TEST_START(TEST_NAME);
    for (int i = 0; i < BLEND_LANES; i++) {
        a.BLEND_FIELD[i] = (__typeof__(a.BLEND_FIELD[i]))(0x100U + (unsigned)i);
        b.BLEND_FIELD[i] = (__typeof__(b.BLEND_FIELD[i]))(0x200U + (unsigned)i);
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     TEST_INSN " $0xaa, %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < BLEND_LANES; i++)
        TEST_ASSERT(out.BLEND_FIELD[i] == ((i & 1) ? b.BLEND_FIELD[i] : a.BLEND_FIELD[i]),
                    TEST_NAME " alternating lane %d", i);
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\tmovdqu %2, %%xmm0\n\t"
                     TEST_INSN " $0xff, %3, %%xmm0, %%xmm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(a), "m"(b)
                     : "xmm0", "ymm1");
    for (int i = 0; i < BLEND_XMM_LANES; i++)
        TEST_ASSERT(out.BLEND_FIELD[i] == b.BLEND_FIELD[i], TEST_NAME " xmm lane %d", i);
    for (int i = BLEND_XMM_LANES; i < BLEND_LANES; i++)
        TEST_ASSERT(out.BLEND_FIELD[i] == 0, TEST_NAME " VEX upper lane %d", i);
    TEST_END();
}

#endif
