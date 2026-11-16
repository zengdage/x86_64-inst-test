#ifndef X86_64_AVX_MINMAX_INT_TEST_H
#define X86_64_AVX_MINMAX_INT_TEST_H

#include "../common.h"

int main(void) {
    ymm_t a, b, out;
    TEST_START(TEST_NAME);
#if TEST_BITS == 8
    for (int i = 0; i < 32; i++) {
        a.u8[i] = (uint8_t)(i * 11);
        b.u8[i] = (uint8_t)(255 - i * 7);
    }
#else
    for (int i = 0; i < 8; i++) {
        a.i32[i] = i == 0 ? INT32_MIN : i == 1 ? INT32_MAX : i * 100 - 350;
        b.i32[i] = i == 0 ? INT32_MAX : i == 1 ? INT32_MIN : 250 - i * 90;
    }
#endif
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
#if TEST_BITS == 8
    for (int i = 0; i < 32; i++) {
        uint8_t expected = TEST_MAX ? (a.u8[i] > b.u8[i] ? a.u8[i] : b.u8[i])
                                    : (a.u8[i] < b.u8[i] ? a.u8[i] : b.u8[i]);
        TEST_ASSERT(out.u8[i] == expected, TEST_NAME " lane %d", i);
    }
#else
    for (int i = 0; i < 8; i++) {
        int32_t expected = TEST_MAX ? (a.i32[i] > b.i32[i] ? a.i32[i] : b.i32[i])
                                    : (a.i32[i] < b.i32[i] ? a.i32[i] : b.i32[i]);
        TEST_ASSERT(out.i32[i] == expected, TEST_NAME " lane %d", i);
    }
#endif
    TEST_END();
}

#endif
