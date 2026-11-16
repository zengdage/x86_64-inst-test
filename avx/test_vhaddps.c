/* TARGET: VHADDPS */
#include "../common.h"

int main(void) {
    TEST_START("VHADDPS");
    const ymm_t a = {.f32 = {1, 2, 4, 8, 16, 32, 64, 128}};
    const ymm_t b = {.f32 = {-1, -2, -4, -8, -16, -32, -64, -128}};
    const float expected[8] = {3, 12, -3, -12, 48, 192, -48, -192};
    ymm_t out;
    __asm__ volatile("vmovaps %1, %%ymm0\n\tvmovaps %2, %%ymm1\n\t"
                     "vhaddps %%ymm1, %%ymm0, %%ymm2\n\tvmovaps %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f32[i] == expected[i], "VHADDPS lane %d", i);
    TEST_END();
}
