#ifndef X86_64_AVX512_MOV_HILO_REG_TEST_H
#define X86_64_AVX512_MOV_HILO_REG_TEST_H

#include "avx512_test.h"

int main(void) {
    xmm_t src1, src2, out;
    src1.u64[0] = UINT64_C(0x1111222233334444);
    src1.u64[1] = UINT64_C(0x5555666677778888);
    src2.u64[0] = UINT64_C(0x9999aaaabbbbcccc);
    src2.u64[1] = UINT64_C(0xddddeeeeffff0000);

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;
    __asm__ volatile("vmovdqa64 %1, %%xmm16\n\t"
                     "vmovdqa64 %2, %%xmm17\n\t"
                     TEST_INSN " %%xmm17, %%xmm16, %%xmm18\n\t"
                     "vmovdqa64 %%xmm18, %0"
                     : "=m"(out) : "m"(src1), "m"(src2)
                     : "xmm16", "xmm17", "xmm18");
#if TEST_HIGH_TO_LOW
    TEST_ASSERT(out.u64[0] == src2.u64[1],
                TEST_NAME " low qword comes from second source high qword");
    TEST_ASSERT(out.u64[1] == src1.u64[1],
                TEST_NAME " high qword comes from first source high qword");
#else
    TEST_ASSERT(out.u64[0] == src1.u64[0],
                TEST_NAME " low qword comes from first source low qword");
    TEST_ASSERT(out.u64[1] == src2.u64[0],
                TEST_NAME " high qword comes from second source low qword");
#endif
    TEST_END();
}

#endif
