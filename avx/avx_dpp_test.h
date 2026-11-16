#ifndef X86_64_AVX_DPP_TEST_H
#define X86_64_AVX_DPP_TEST_H

#include "../common.h"

#if TEST_BITS == 32
#define DPP_FIELD f32
#define DPP_LANES 8
#define DPP_REG "ymm"
#else
#define DPP_FIELD f64
#define DPP_LANES 2
#define DPP_REG "xmm"
#endif

int main(void) {
    ymm_t a = {0}, b = {0}, out;
    TEST_START(TEST_NAME);
    for (int i = 0; i < DPP_LANES; i++) {
        a.DPP_FIELD[i] = (__typeof__(a.DPP_FIELD[i]))((i & 3) + 1);
        b.DPP_FIELD[i] = (__typeof__(b.DPP_FIELD[i]))((i & 3) + 5);
    }
    __asm__ volatile("vmovdqu %1, %%" DPP_REG "0\n\t"
                     "vmovdqu %2, %%" DPP_REG "1\n\t"
                     TEST_INSN " $0xff, %%" DPP_REG "1, %%" DPP_REG "0, %%" DPP_REG "2\n\t"
                     "vmovdqu %%" DPP_REG "2, %0"
                     : "=m"(out) : "m"(a), "m"(b)
                     : DPP_REG "0", DPP_REG "1", DPP_REG "2");
#if TEST_BITS == 32
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f32[i] == 70.0f, TEST_NAME " full mask lane %d", i);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     TEST_INSN " $0x81, %2, %%ymm0, %%ymm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm2");
    for (int i = 0; i < 8; i++) {
        float expected = i == 0 ? 32.0f : i == 4 ? 32.0f : 0.0f;
        TEST_ASSERT(out.f32[i] == expected, TEST_NAME " lane-local mask lane %d", i);
    }
#else
    TEST_ASSERT(out.f64[0] == 17.0 && out.f64[1] == 17.0,
                TEST_NAME " full input/output mask");
    __asm__ volatile("vmovdqu %1, %%xmm0\n\t"
                     TEST_INSN " $0x21, %2, %%xmm0, %%xmm2\n\t"
                     "vmovdqu %%xmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm2");
    TEST_ASSERT(out.f64[0] == 12.0 && out.u64[1] == 0,
                TEST_NAME " lane selector and output mask");
#endif
    TEST_END();
}

#endif
