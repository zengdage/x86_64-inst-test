/* TARGET: VLDDQU */
#include "../common.h"

int main(void) {
    TEST_START("VLDDQU");
    uint8_t storage[33];
    for (int i = 0; i < 33; i++) storage[i] = (uint8_t)(i * 7 + 3);
    ymm_t out;
    __asm__ volatile("vlddqu %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(*(const uint8_t (*)[32])(storage + 1))
                     : "ymm0", "memory");
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(out.u8[i] == storage[i + 1], "VLDDQU byte %d", i);
    TEST_END();
}
