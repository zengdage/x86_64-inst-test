/* TARGET: VPCOMPRESSW */
#include "../include/avx512_test.h"

static int selected(uint64_t mask, int lane) {
    return (int)((mask >> lane) & 1U);
}

int main(void) {
    TEST_START("VPCOMPRESSW");
    if (!CHECK_AVX512F() || !CHECK_AVX512BW()) return 0;
    zmm_test_t src, dst;
    for (int i = 0; i < 32; i++) src.u16[i] = (uint16_t)(0x1000U + (uint32_t)i);
    const uint64_t mask = UINT64_C(0x8429);
    __asm__ volatile("kmovq %2, %%k1\n\t"
                     "vmovdqu16 %1, %%zmm0\n\t"
                     "vpcompressw %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqu16 %%zmm1, %0"
                     : "=m"(dst) : "m"(src), "r"(mask) : "zmm0", "zmm1", "k1");
    int packed = 0;
    int count = 0;
    for (int i = 0; i < 32; i++) count += selected(mask, i);
    for (int i = 0; i < 32; i++) {
        while (packed < 32 && !selected(mask, packed)) packed++;
        uint16_t expected = (i < count) ? src.u16[packed++] : 0;
        TEST_ASSERT(dst.u16[i] == expected, "VPCOMPRESSW lane %d", i);
    }
    TEST_END();
}
