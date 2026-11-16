#ifndef X86_64_AVX_MOVMSK_TEST_H
#define X86_64_AVX_MOVMSK_TEST_H

#include "../common.h"

int main(void) {
    ymm_t src = {0};
    unsigned result;
    TEST_START(TEST_NAME);
#if TEST_BITS == 32
    for (int i = 0; i < 8; i++) src.u32[i] = (UINT32_C(0xa5) >> i) & 1U ? UINT32_C(0x80000000) : 0;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovmskps %%ymm0, %0" : "=r"(result) : "m"(src) : "ymm0");
    TEST_ASSERT(result == 0xa5U, "VMOVMSKPS ymm %#x", result);
    __asm__ volatile("movdqu %1, %%xmm0\n\tvmovmskps %%xmm0, %0" : "=r"(result) : "m"(src) : "xmm0");
    TEST_ASSERT(result == 5U, "VMOVMSKPS xmm %#x", result);
#else
    for (int i = 0; i < 4; i++) src.u64[i] = (UINT32_C(0x9) >> i) & 1U ? UINT64_C(0x8000000000000000) : 0;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovmskpd %%ymm0, %0" : "=r"(result) : "m"(src) : "ymm0");
    TEST_ASSERT(result == 9U, "VMOVMSKPD ymm %#x", result);
    __asm__ volatile("movdqu %1, %%xmm0\n\tvmovmskpd %%xmm0, %0" : "=r"(result) : "m"(src) : "xmm0");
    TEST_ASSERT(result == 1U, "VMOVMSKPD xmm %#x", result);
#endif
    TEST_END();
}

#endif
