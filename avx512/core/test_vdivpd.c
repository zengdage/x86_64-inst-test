/* TARGET: VDIVPD */
#include "../include/avx512_test.h"
#include <math.h>

int main(void) {
    TEST_START("VDIVPD");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t a, b, out;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)((i + 1) * 8);
        b.f64[i] = (double)(i + 1);
    }
    a.f64[0] = 1.0; b.f64[0] = 0.0;
    a.f64[1] = -1.0; b.f64[1] = 0.0;
    a.f64[2] = 0.0; b.f64[2] = 0.0;
    a.f64[3] = INFINITY; b.f64[3] = INFINITY;
    a.f64[4] = 1.0; b.f64[4] = INFINITY;
    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vdivpd %%zmm1, %%zmm0, %%zmm2\n\tvmovapd %%zmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(isinf(out.f64[0]) && !signbit(out.f64[0]), "VDIVPD 1/+0");
    TEST_ASSERT(isinf(out.f64[1]) && signbit(out.f64[1]), "VDIVPD -1/+0");
    TEST_ASSERT(IS_QNAN(out.f64[2]) && IS_QNAN(out.f64[3]), "VDIVPD invalid");
    TEST_ASSERT(out.f64[4] == 0.0 && !signbit(out.f64[4]), "VDIVPD 1/+Inf");
    for (int i = 5; i < 8; i++)
        TEST_ASSERT(out.f64[i] == 8.0, "VDIVPD lane %d", i);
    TEST_END();
}
