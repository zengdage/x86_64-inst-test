/* AVX-512BW word test-to-mask. */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("AVX-512BW VPTESTMW");
    if (!CHECK_AVX512BW()) return 0;
    zmm_test_t a, b;
    uint64_t got, expected = 0;
    for (int i = 0; i < 32; i++) {
        a.u16[i] = (uint16_t)(i * 37U + 3U);
        b.u16[i] = (uint16_t)(i * 53U + 7U);
        expected |= (uint64_t)((a.u16[i] & b.u16[i]) != 0) << i;
    }
    __asm__ volatile("vmovdqu64 %1, %%zmm0\n\tvmovdqu64 %2, %%zmm1\n\t"
                     "vptestmw %%zmm1, %%zmm0, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(got) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT((uint32_t)got == (uint32_t)expected,
                "VPTESTMW mask %08x != %08x", (uint32_t)got, (uint32_t)expected);
    TEST_END();
}
