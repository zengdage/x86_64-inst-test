/* TARGET: VHSUBPS */
#include "../common.h"

int main(void) {
    TEST_START("VHSUBPS");
    const ymm_t a = {.f32 = {9, 2, 20, 5, 40, 8, 80, 16}};
    const ymm_t b = {.f32 = {-1, -3, -4, -12, -16, -32, -64, -128}};
    const float expected[8] = {7, 15, 2, 8, 32, 64, 16, 64};
    ymm_t out;
    __asm__ volatile("vmovaps %1, %%ymm0\n\tvmovaps %2, %%ymm1\n\t"
                     "vhsubps %%ymm1, %%ymm0, %%ymm2\n\tvmovaps %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f32[i] == expected[i], "VHSUBPS lane %d", i);
    TEST_END();
}
