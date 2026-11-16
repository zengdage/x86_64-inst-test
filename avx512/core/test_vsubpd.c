/* TARGET: VSUBPD */
#include "../include/avx512_test.h"
#include <float.h>
#include <math.h>

int main(void) {
    TEST_START("VSUBPD");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t a, b, out;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)(i + 10) * 2.0;
        b.f64[i] = (double)(i + 1) * 0.5;
    }
    a.f64[0] = INFINITY; b.f64[0] = INFINITY;
    a.f64[1] = -0.0; b.f64[1] = 0.0;
    a.f64[2] = DBL_MAX; b.f64[2] = -DBL_MAX;
    a.f64[3] = DBL_MIN; b.f64[3] = DBL_MIN / 2.0;
    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vsubpd %%zmm1, %%zmm0, %%zmm2\n\tvmovapd %%zmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(IS_QNAN(out.f64[0]), "VSUBPD Inf-Inf");
    TEST_ASSERT(out.f64[1] == 0.0 && signbit(out.f64[1]), "VSUBPD signed zero");
    TEST_ASSERT(isinf(out.f64[2]) && !signbit(out.f64[2]), "VSUBPD overflow");
    TEST_ASSERT(out.f64[3] == DBL_MIN / 2.0, "VSUBPD subnormal");
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(out.f64[i] == a.f64[i] - b.f64[i], "VSUBPD lane %d", i);
    TEST_END();
}
