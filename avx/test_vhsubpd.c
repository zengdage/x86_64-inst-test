/* TARGET: VHSUBPD */
#include "../common.h"

int main(void) {
    TEST_START("VHSUBPD");
    const ymm_t a = {.f64 = {9, 2, 40, 8}};
    const ymm_t b = {.f64 = {-1, -3, -16, -32}};
    const double expected[4] = {7, 2, 32, 16};
    ymm_t out;
    __asm__ volatile("vmovapd %1, %%ymm0\n\tvmovapd %2, %%ymm1\n\t"
                     "vhsubpd %%ymm1, %%ymm0, %%ymm2\n\tvmovapd %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.f64[i] == expected[i], "VHSUBPD lane %d", i);
    TEST_END();
}
