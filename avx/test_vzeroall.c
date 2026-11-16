/* TARGET: VZEROALL */
#include "../common.h"

int main(void) {
    ymm_t src, out0, out1;
    TEST_START("VZEROALL");
    memset(&src, 0xa5, sizeof(src));
    __asm__ volatile("vmovdqu %2, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vzeroall\n\tvmovdqu %%ymm0, %0\n\tvmovdqu %%ymm1, %1"
                     : "=m"(out0), "=m"(out1) : "m"(src) : "ymm0", "ymm1");
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(out0.u64[i] == 0, "VZEROALL ymm0 lane %d", i);
        TEST_ASSERT(out1.u64[i] == 0, "VZEROALL ymm1 lane %d", i);
    }
    TEST_END();
}
