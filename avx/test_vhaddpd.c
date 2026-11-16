/* TARGET: VHADDPD */
#include "../common.h"

int main(void) {
    TEST_START("VHADDPD");
    const ymm_t a = {.f64 = {1, 2, 16, 32}};
    const ymm_t b = {.f64 = {-1, -2, -16, -32}};
    const double expected[4] = {3, -3, 48, -48};
    ymm_t out;
    __asm__ volatile("vmovapd %1, %%ymm0\n\tvmovapd %2, %%ymm1\n\t"
                     "vhaddpd %%ymm1, %%ymm0, %%ymm2\n\tvmovapd %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.f64[i] == expected[i], "VHADDPD lane %d", i);
    TEST_END();
}
