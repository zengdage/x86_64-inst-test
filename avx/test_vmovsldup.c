/* TARGET: VMOVSLDUP */
#include "../common.h"

int main(void) {
    TEST_START("VMOVSLDUP");
    const ymm_t src = {.u32 = {0, 1, 2, 3, 4, 5, 6, 7}};
    const uint32_t expected[8] = {0, 0, 2, 2, 4, 4, 6, 6};
    ymm_t out;
    __asm__ volatile("vmovsldup %1, %%ymm0\n\tvmovdqa %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == expected[i], "VMOVSLDUP lane %d", i);
    TEST_END();
}
