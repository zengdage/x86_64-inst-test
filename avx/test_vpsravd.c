/* TARGET: VPSRAVD */
#include "../common.h"

static uint32_t arithmetic_right(uint32_t value, uint32_t count) {
    if (count >= 32) return (value & UINT32_C(0x80000000)) ? UINT32_MAX : 0;
    if (count == 0) return value;
    uint32_t shifted = value >> count;
    if (value & UINT32_C(0x80000000))
        shifted |= UINT32_MAX << (32 - count);
    return shifted;
}

int main(void) {
    TEST_START("VPSRAVD");
    ymm_t src = {.u32 = {
        UINT32_C(0x80000000), UINT32_C(0x80000001), UINT32_C(0x7fffffff), UINT32_MAX,
        UINT32_C(0x12345678), UINT32_C(0xaaaaaaaa), UINT32_C(0x55555555), UINT32_C(0xf000000f)
    }};
    ymm_t counts = {.u32 = {0, 1, 31, 32, 33, 255, UINT32_C(0x80000000), 4}};
    ymm_t out;
    __asm__ volatile("vmovdqa %1, %%ymm0\n\tvmovdqa %2, %%ymm1\n\t"
                     "vpsravd %%ymm1, %%ymm0, %%ymm2\n\tvmovdqa %%ymm2, %0"
                     : "=m"(out) : "m"(src), "m"(counts)
                     : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == arithmetic_right(src.u32[i], counts.u32[i]),
                    "VPSRAVD lane %d", i);
    TEST_END();
}
