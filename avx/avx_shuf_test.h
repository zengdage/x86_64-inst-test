#ifndef X86_64_AVX_SHUF_TEST_H
#define X86_64_AVX_SHUF_TEST_H

#include "../common.h"

int main(void) {
    ymm_t a = { .u32 = {0,1,2,3,4,5,6,7} };
    ymm_t b = { .u32 = {10,11,12,13,14,15,16,17} };
    ymm_t out, initial;
    TEST_START(TEST_NAME);
#if TEST_BITS == 32
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     "vshufps $0x1b, %2, %%ymm0, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    const uint32_t expected[8] = {3,2,11,10,7,6,15,14};
    for (int i = 0; i < 8; i++) TEST_ASSERT(out.u32[i] == expected[i], "VSHUFPS lane %d", i);
#else
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     "vshufpd $0x5, %2, %%ymm0, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    TEST_ASSERT(out.u64[0] == a.u64[1] && out.u64[1] == b.u64[0] &&
                out.u64[2] == a.u64[3] && out.u64[3] == b.u64[2],
                "VSHUFPD lane-local selectors");
#endif
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\tmovdqu %2, %%xmm0\n\t"
                     TEST_INSN " $0xff, %3, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(a), "m"(b)
                     : "xmm0", "ymm1");
#if TEST_BITS == 32
    TEST_ASSERT(out.u32[0] == 3 && out.u32[1] == 3 && out.u32[2] == 13 && out.u32[3] == 13,
                "VSHUFPS immediate endpoints");
    for (int i = 4; i < 8; i++) TEST_ASSERT(out.u32[i] == 0, "VSHUFPS upper lane %d", i);
#else
    TEST_ASSERT(out.u64[0] == a.u64[1] && out.u64[1] == b.u64[1],
                "VSHUFPD immediate endpoints");
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0, "VSHUFPD upper YMM zero");
#endif
    TEST_END();
}

#endif
