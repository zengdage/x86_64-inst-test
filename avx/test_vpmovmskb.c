/* TARGET: VPMOVMSKB */
#include "../common.h"

int main(void) {
    ymm_t value;
    unsigned result;
    TEST_START("VPMOVMSKB");
    for (int i = 0; i < 32; i++)
        value.u8[i] = (uint8_t)(((UINT32_C(0xa5810042) >> i) & 1U) ?
                               (0x80U | (unsigned)i) : i);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmovmskb %%ymm0, %0"
                     : "=r"(result) : "m"(value) : "ymm0");
    TEST_ASSERT(result == UINT32_C(0xa5810042),
                "VPMOVMSKB ymm expected %#x got %#x",
                UINT32_C(0xa5810042), result);
    __asm__ volatile("movdqu %1, %%xmm0\n\tvpmovmskb %%xmm0, %0"
                     : "=r"(result) : "m"(value) : "xmm0");
    TEST_ASSERT(result == UINT32_C(0x0042),
                "VPMOVMSKB xmm expected 0x42 got %#x", result);
    TEST_END();
}
