/* TARGET: VPSRLVD */
#include "../common.h"

int main(void) {
    TEST_START("VPSRLVD");
    ymm_t src = {.u32 = {
        UINT32_C(0x80000000), UINT32_C(0x80000001), UINT32_MAX, UINT32_C(0x12345678),
        UINT32_C(0xaaaaaaaa), UINT32_C(0x55555555), UINT32_C(0xf000000f), UINT32_C(0x10203040)
    }};
    ymm_t counts = {.u32 = {0, 1, 31, 32, 33, 255, UINT32_C(0x80000000), 4}};
    ymm_t out;
    __asm__ volatile("vmovdqa %1, %%ymm0\n\tvmovdqa %2, %%ymm1\n\t"
                     "vpsrlvd %%ymm1, %%ymm0, %%ymm2\n\tvmovdqa %%ymm2, %0"
                     : "=m"(out) : "m"(src), "m"(counts)
                     : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++) {
        uint32_t expected = counts.u32[i] >= 32 ? 0 : src.u32[i] >> counts.u32[i];
        TEST_ASSERT(out.u32[i] == expected, "VPSRLVD lane %d", i);
    }
    TEST_END();
}
