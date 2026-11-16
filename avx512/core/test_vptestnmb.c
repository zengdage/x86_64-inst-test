/* AVX-512BW byte inverted test-to-mask. */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("AVX-512BW VPTESTNMB");
    if (!CHECK_AVX512BW()) return 0;
    zmm_test_t a, b;
    uint64_t got, expected = 0;
    for (int i = 0; i < 64; i++) {
        a.u8[i] = (uint8_t)(i * 37U + 3U);
        b.u8[i] = (uint8_t)(i * 53U + 7U);
        expected |= (uint64_t)((a.u8[i] & b.u8[i]) == 0) << i;
    }
    __asm__ volatile("vmovdqu64 %1, %%zmm0\n\tvmovdqu64 %2, %%zmm1\n\t"
                     "vptestnmb %%zmm1, %%zmm0, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(got) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(got == expected, "VPTESTNMB full mask");
    const uint64_t gate = UINT64_C(0x8000000180000021);
    __asm__ volatile("kmovq %3, %%k2\n\tvmovdqu64 %1, %%zmm0\n\t"
                     "vmovdqu64 %2, %%zmm1\n\tvptestnmb %%zmm1, %%zmm0, %%k1%{%%k2%}\n\t"
                     "kmovq %%k1, %0"
                     : "=r"(got) : "m"(a), "m"(b), "r"(gate)
                     : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT(got == (expected & gate), "VPTESTNMB input writemask");
    TEST_END();
}
