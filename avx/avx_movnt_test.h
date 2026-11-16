#ifndef X86_64_AVX_MOVNT_TEST_H
#define X86_64_AVX_MOVNT_TEST_H

#include "../common.h"

int main(void) {
    ymm_t src, out;
    TEST_START(TEST_NAME);
    for (int i = 0; i < 32; i++) src.u8[i] = (uint8_t)(0xf0U - (unsigned)i);
#if TEST_LOAD
    __asm__ volatile(TEST_INSN " %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0", "memory");
#else
    memset(&out, 0, sizeof(out));
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" TEST_INSN " %%ymm0, %0\n\tsfence"
                     : "=m"(out) : "m"(src) : "ymm0", "memory");
#endif
    TEST_ASSERT(memcmp(&out, &src, sizeof(src)) == 0, TEST_NAME " exact 256-bit transfer");
    TEST_END();
}

#endif
