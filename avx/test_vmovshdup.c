/* TARGET: VMOVSHDUP */
#include "../common.h"

int main(void) {
    TEST_START("VMOVSHDUP");
    const ymm_t src = {.u32 = {0, 1, 2, 3, 4, 5, 6, 7}};
    const uint32_t expected[8] = {1, 1, 3, 3, 5, 5, 7, 7};
    ymm_t out;
    __asm__ volatile("vmovshdup %1, %%ymm0\n\tvmovdqa %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == expected[i], "VMOVSHDUP lane %d", i);
    TEST_END();
}
