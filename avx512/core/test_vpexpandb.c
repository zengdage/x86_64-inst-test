/* TARGET: VPEXPANDB */
#include "../include/avx512_test.h"

static int selected(uint64_t mask, int lane) {
    return (int)((mask >> lane) & 1U);
}

int main(void) {
    TEST_START("VPEXPANDB");
    if (!CHECK_AVX512F() || !CHECK_AVX512BW()) return 0;
    zmm_test_t src, dst;
    for (int i = 0; i < 64; i++) src.u8[i] = (uint8_t)(0x40U + (uint32_t)i);
    const uint64_t mask = UINT64_C(0x03030507090b0d0f);
    __asm__ volatile("kmovq %2, %%k1\n\t"
                     "vmovdqu8 %1, %%zmm0\n\t"
                     "vpexpandb %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqu8 %%zmm1, %0"
                     : "=m"(dst) : "m"(src), "r"(mask) : "zmm0", "zmm1", "k1");
    int consumed = 0;
    int expected_count = 0;
    for (int i = 0; i < 64; i++) {
        uint8_t expected = selected(mask, i) ? src.u8[consumed++] : 0;
        expected_count += selected(mask, i);
        TEST_ASSERT(dst.u8[i] == expected, "VPEXPANDB lane %d", i);
    }
    TEST_ASSERT(consumed == expected_count, "VPEXPANDB consumed %d bytes", consumed);
    TEST_END();
}
