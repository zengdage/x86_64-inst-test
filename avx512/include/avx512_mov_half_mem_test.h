#ifndef X86_64_AVX512_MOV_HALF_MEM_TEST_H
#define X86_64_AVX512_MOV_HALF_MEM_TEST_H

#include "avx512_test.h"

int main(void) {
    xmm_t src, out;
    uint64_t memory = UINT64_C(0x0123456789abcdef);
    uint64_t stored = UINT64_C(0xdeadbeefcafebabe);
    src.u64[0] = UINT64_C(0x1111222233334444);
    src.u64[1] = UINT64_C(0xaaaabbbbccccdddd);

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;
    __asm__ volatile("vmovdqa64 %1, %%xmm16\n\t"
                     TEST_INSN " %2, %%xmm16, %%xmm17\n\t"
                     "vmovdqa64 %%xmm17, %0"
                     : "=m"(out) : "m"(src), "m"(memory)
                     : "xmm16", "xmm17");
#if TEST_HIGH
    TEST_ASSERT(out.u64[0] == src.u64[0], TEST_NAME " load keeps low qword");
    TEST_ASSERT(out.u64[1] == memory, TEST_NAME " load replaces high qword");
#else
    TEST_ASSERT(out.u64[0] == memory, TEST_NAME " load replaces low qword");
    TEST_ASSERT(out.u64[1] == src.u64[1], TEST_NAME " load keeps high qword");
#endif

    __asm__ volatile("vmovdqa64 %1, %%xmm16\n\t"
                     TEST_INSN " %%xmm16, %0"
                     : "=m"(stored) : "m"(src) : "xmm16", "memory");
    TEST_ASSERT(stored == src.u64[TEST_HIGH ? 1 : 0],
                TEST_NAME " store selected qword");
    TEST_END();
}

#endif
