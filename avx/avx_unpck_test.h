#ifndef X86_64_AVX_UNPCK_TEST_H
#define X86_64_AVX_UNPCK_TEST_H

#include "../common.h"

#if TEST_BITS == 32
#define UNPCK_FIELD u32
#define UNPCK_LANES 8
#define UNPCK_XMM_LANES 4
#else
#define UNPCK_FIELD u64
#define UNPCK_LANES 4
#define UNPCK_XMM_LANES 2
#endif

static unsigned source_index(int output_lane) {
    int lane_base = (output_lane / UNPCK_XMM_LANES) * UNPCK_XMM_LANES;
    int pair = (output_lane % UNPCK_XMM_LANES) / 2;
    return (unsigned)(lane_base + pair + (TEST_HIGH ? UNPCK_XMM_LANES / 2 : 0));
}

int main(void) {
    ymm_t a = {0}, b = {0}, out, initial;
    TEST_START(TEST_NAME);
    for (int i = 0; i < UNPCK_LANES; i++) {
        a.UNPCK_FIELD[i] = (__typeof__(a.UNPCK_FIELD[i]))(0x10U + (unsigned)i);
        b.UNPCK_FIELD[i] = (__typeof__(b.UNPCK_FIELD[i]))(0x80U + (unsigned)i);
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < UNPCK_LANES; i++) {
        unsigned index = source_index(i);
        __typeof__(out.UNPCK_FIELD[i]) expected = (i & 1) ? b.UNPCK_FIELD[index] : a.UNPCK_FIELD[index];
        TEST_ASSERT(out.UNPCK_FIELD[i] == expected, TEST_NAME " ymm lane %d", i);
    }
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm2\n\tmovdqu %2, %%xmm0\n\t"
                     TEST_INSN " %3, %%xmm0, %%xmm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(initial), "m"(a), "m"(b) : "xmm0", "ymm2");
    for (int i = 0; i < UNPCK_XMM_LANES; i++) {
        unsigned index = source_index(i);
        __typeof__(out.UNPCK_FIELD[i]) expected = (i & 1) ? b.UNPCK_FIELD[index] : a.UNPCK_FIELD[index];
        TEST_ASSERT(out.UNPCK_FIELD[i] == expected, TEST_NAME " xmm lane %d", i);
    }
    for (int i = UNPCK_XMM_LANES; i < UNPCK_LANES; i++)
        TEST_ASSERT(out.UNPCK_FIELD[i] == 0, TEST_NAME " upper lane %d", i);
    TEST_END();
}

#endif
