/* AVX-512VBMI2 variable word funnel shift right. */
#include "../include/avx512_test.h"

static uint16_t reference(uint16_t high, uint16_t low, unsigned count) {
    count &= 15U;
    return count ? (uint16_t)(((uint32_t)high >> count) |
                              ((uint32_t)low << (16U - count))) : high;
}

int main(void) {
    TEST_START("AVX-512VBMI2 VPSHRDVW");
    if (!CHECK_AVX512VBMI2()) return 0;
    static const uint16_t counts[32] = {
        0, 1, 7, 15, 16, 17, 31, UINT16_MAX, 2, 3, 4, 5, 6, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 32, 255, 0, 1, 14, 15, 16, 17, 0x8000, UINT16_MAX
    };
    zmm_test_t high, low, count, out, zeroed;
    for (int i = 0; i < 32; i++) {
        high.u16[i] = (uint16_t)(0x8001U ^ (uint16_t)(i * 0x1111U));
        low.u16[i] = (uint16_t)(0x1234U + (uint16_t)(i * 0x0101U));
        count.u16[i] = counts[i];
    }
    __asm__ volatile("vmovdqu16 %1, %%zmm0\n\tvmovdqu16 %2, %%zmm1\n\t"
                     "vmovdqu16 %3, %%zmm2\n\tvpshrdvw %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqu16 %%zmm0, %0"
                     : "=m"(out) : "m"(high), "m"(low), "m"(count)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(out.u16[i] == reference(high.u16[i], low.u16[i], counts[i]),
                    "VPSHRDVW lane %d count %u", i, counts[i]);

    const uint32_t mask = UINT32_C(0x80018021);
    __asm__ volatile("kmovd %2, %%k1\n\tvmovdqu16 %1, %%zmm0\n\t"
                     "vmovdqu16 %3, %%zmm1\n\tvmovdqu16 %4, %%zmm2\n\t"
                     "vpshrdvw %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
                     "vmovdqu16 %%zmm0, %0"
                     : "=m"(zeroed) : "m"(high), "r"(mask), "m"(low), "m"(count)
                     : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 32; i++) {
        uint16_t expected = ((mask >> i) & 1U)
            ? reference(high.u16[i], low.u16[i], counts[i]) : 0;
        TEST_ASSERT(zeroed.u16[i] == expected, "VPSHRDVW zero mask lane %d", i);
    }
    TEST_END();
}
