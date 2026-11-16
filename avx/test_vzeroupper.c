/* TARGET: VZEROUPPER */
#include "../common.h"

int main(void) {
    ymm_t src = { .u64 = {1, 2, 3, 4} }, out0, out1;
    TEST_START("VZEROUPPER");
    __asm__ volatile("vmovdqu %2, %%ymm2\n\tvmovdqu %2, %%ymm3\n\t"
                     "vzeroupper\n\tvmovdqu %%ymm2, %0\n\tvmovdqu %%ymm3, %1"
                     : "=m"(out0), "=m"(out1) : "m"(src) : "ymm2", "ymm3");
    for (int i = 0; i < 2; i++) {
        TEST_ASSERT(out0.u64[i] == src.u64[i], "VZEROUPPER ymm2 low %d", i);
        TEST_ASSERT(out1.u64[i] == src.u64[i], "VZEROUPPER ymm3 low %d", i);
    }
    for (int i = 2; i < 4; i++) {
        TEST_ASSERT(out0.u64[i] == 0, "VZEROUPPER ymm2 high %d", i);
        TEST_ASSERT(out1.u64[i] == 0, "VZEROUPPER ymm3 high %d", i);
    }
    TEST_END();
}
