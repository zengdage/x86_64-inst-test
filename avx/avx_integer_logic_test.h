#ifndef X86_64_AVX_INTEGER_LOGIC_TEST_H
#define X86_64_AVX_INTEGER_LOGIC_TEST_H

#include "../common.h"

#define LOGIC_AND 1
#define LOGIC_ANDN 2
#define LOGIC_OR 3
#define LOGIC_XOR 4

static uint64_t expected(uint64_t a, uint64_t b) {
#if TEST_LOGIC == LOGIC_AND
    return a & b;
#elif TEST_LOGIC == LOGIC_ANDN
    return ~a & b;
#elif TEST_LOGIC == LOGIC_OR
    return a | b;
#elif TEST_LOGIC == LOGIC_XOR
    return a ^ b;
#else
#error "Unknown TEST_LOGIC"
#endif
}

int main(void) {
    const ymm_t a = { .u64 = {
        UINT64_C(0x0123456789abcdef), UINT64_C(0xffff0000aaaa5555),
        UINT64_C(0x8000000000000001), UINT64_C(0x55aa55aa0f0ff0f0)
    } };
    const ymm_t b = { .u64 = {
        UINT64_C(0xfedcba9876543210), UINT64_C(0x123456789abcdef0),
        UINT64_C(0x7fffffffffffffff), UINT64_C(0xaa55aa55f0f00f0f)
    } };
    ymm_t out, initial;

    TEST_START(TEST_NAME);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " %%ymm1, %%ymm0, %%ymm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b)
                     : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == expected(a.u64[i], b.u64[i]),
                    TEST_NAME " ymm lane %d", i);

    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     TEST_INSN " %2, %%ymm0, %%ymm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm2");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == expected(a.u64[i], b.u64[i]),
                    TEST_NAME " memory lane %d", i);

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm2\n\tmovdqu %2, %%xmm0\n\t"
                     "movdqu %3, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(initial), "m"(a), "m"(b)
                     : "xmm0", "xmm1", "ymm2");
    for (int i = 0; i < 2; i++)
        TEST_ASSERT(out.u64[i] == expected(a.u64[i], b.u64[i]),
                    TEST_NAME " xmm lane %d", i);
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0,
                TEST_NAME " VEX.128 zeroes upper YMM");
    TEST_END();
}

#endif
