#ifndef X86_64_AVX_BROADCAST_TEST_H
#define X86_64_AVX_BROADCAST_TEST_H

#include "../common.h"

#if TEST_WHOLE128
int main(void) {
    xmm_t src = { .u64 = {UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210)} };
    ymm_t out;
    TEST_START(TEST_NAME);
    __asm__ volatile(TEST_INSN " %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    TEST_ASSERT(out.u64[0] == src.u64[0] && out.u64[1] == src.u64[1] &&
                out.u64[2] == src.u64[0] && out.u64[3] == src.u64[1],
                TEST_NAME " duplicates complete 128-bit source");
    TEST_END();
}
#else
#if TEST_ELEM_BITS == 8
#define BROADCAST_FIELD u8
#define BROADCAST_LANES 32
typedef uint8_t broadcast_t;
#elif TEST_ELEM_BITS == 16
#define BROADCAST_FIELD u16
#define BROADCAST_LANES 16
typedef uint16_t broadcast_t;
#elif TEST_ELEM_BITS == 32
#define BROADCAST_FIELD u32
#define BROADCAST_LANES 8
typedef uint32_t broadcast_t;
#else
#define BROADCAST_FIELD u64
#define BROADCAST_LANES 4
typedef uint64_t broadcast_t;
#endif

int main(void) {
    const broadcast_t value = (broadcast_t)
#if TEST_ELEM_BITS == 8
        UINT8_C(0xa5);
#elif TEST_ELEM_BITS == 16
        UINT16_C(0xa55a);
#elif TEST_ELEM_BITS == 32
        UINT32_C(0x89abcdef);
#else
        UINT64_C(0x0123456789abcdef);
#endif
    xmm_t src = {0};
    ymm_t out;
#if TEST_XMM_DEST
    ymm_t initial;
#endif
    src.BROADCAST_FIELD[0] = value;
    TEST_START(TEST_NAME);
    __asm__ volatile(TEST_INSN " %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(value) : "ymm0");
    for (int i = 0; i < BROADCAST_LANES; i++)
        TEST_ASSERT(out.BROADCAST_FIELD[i] == value, TEST_NAME " memory ymm lane %d", i);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\t" TEST_INSN " %%xmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "xmm0", "ymm1");
    for (int i = 0; i < BROADCAST_LANES; i++)
        TEST_ASSERT(out.BROADCAST_FIELD[i] == value, TEST_NAME " register ymm lane %d", i);
#if TEST_XMM_DEST
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\tvmovdqu %2, %%xmm0\n\t"
                     TEST_INSN " %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(src) : "xmm0", "ymm1");
    for (int i = 0; i < BROADCAST_LANES / 2; i++)
        TEST_ASSERT(out.BROADCAST_FIELD[i] == value, TEST_NAME " xmm lane %d", i);
    for (int i = BROADCAST_LANES / 2; i < BROADCAST_LANES; i++)
        TEST_ASSERT(out.BROADCAST_FIELD[i] == 0, TEST_NAME " upper lane %d", i);
#endif
    TEST_END();
}
#endif

#endif
