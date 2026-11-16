/* TARGET: VADDPD */
#include "../include/avx512_test.h"
#include <float.h>
#include <math.h>

int main(void) {
    TEST_START("VADDPD");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t a, b, initial, merged, zeroed;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)i * 1.5;
        b.f64[i] = (double)(i + 2) * 0.5;
        initial.f64[i] = 99.0;
    }
    a.f64[0] = INFINITY; b.f64[0] = -INFINITY;
    a.f64[1] = -0.0; b.f64[1] = -0.0;
    a.f64[2] = DBL_MAX; b.f64[2] = DBL_MAX;
    uint32_t mask = UINT32_C(0x81);
    __asm__ volatile(
        "kmovb %5, %%k1\n\tvmovapd %3, %%zmm0\n\t"
        "vmovapd %4, %%zmm1\n\tvmovapd %2, %%zmm2\n\t"
        "vaddpd %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\tvmovapd %%zmm2, %0\n\t"
        "vaddpd %%zmm1, %%zmm0, %%zmm3%{%%k1%}%{z%}\n\tvmovapd %%zmm3, %1"
        : "=m"(merged), "=m"(zeroed)
        : "m"(initial), "m"(a), "m"(b), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "zmm3", "k1");
    for (int i = 0; i < 8; i++) {
        if (i == 0 || i == 7) {
            double expected = a.f64[i] + b.f64[i];
            if (i == 0)
                TEST_ASSERT(IS_QNAN(merged.f64[i]), "VADDPD Inf + -Inf lane");
            else
                TEST_ASSERT(merged.f64[i] == expected, "VADDPD lane %d", i);
            TEST_ASSERT(zeroed.u64[i] == merged.u64[i], "VADDPD zero mask active lane %d", i);
        } else {
            TEST_ASSERT(merged.f64[i] == 99.0, "VADDPD merge lane %d", i);
            TEST_ASSERT(zeroed.u64[i] == 0, "VADDPD zero lane %d", i);
        }
    }
    TEST_END();
}
