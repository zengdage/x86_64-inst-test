#ifndef X86_64_AVX_MOV_HALF_TEST_H
#define X86_64_AVX_MOV_HALF_TEST_H

#include "../common.h"

int main(void) {
    xmm_t first = { .u64 = {UINT64_C(0x1111222233334444), UINT64_C(0x5555666677778888)} };
    xmm_t out;
    uint64_t memory = UINT64_C(0xdeadbeef01234567), stored = 0;
    TEST_START(TEST_NAME);
    __asm__ volatile("movdqu %1, %%xmm0\n\t" TEST_INSN " %2, %%xmm0, %%xmm1\n\t"
                     "movdqu %%xmm1, %0"
                     : "=m"(out) : "m"(first), "m"(memory) : "xmm0", "xmm1");
#if TEST_HIGH
    TEST_ASSERT(out.u64[0] == first.u64[0] && out.u64[1] == memory,
                TEST_NAME " memory load high half");
#else
    TEST_ASSERT(out.u64[0] == memory && out.u64[1] == first.u64[1],
                TEST_NAME " memory load low half");
#endif
    __asm__ volatile("movdqu %1, %%xmm0\n\t" TEST_INSN " %%xmm0, %0"
                     : "=m"(stored) : "m"(out) : "xmm0", "memory");
    TEST_ASSERT(stored == memory, TEST_NAME " memory store selected half");
    TEST_END();
}

#endif
