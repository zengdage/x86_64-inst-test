/* TARGET: VMULPD */
#include "../include/avx512_test.h"
#include <float.h>
#include <math.h>

int main(void) {
    TEST_START("VMULPD");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t a, b, out;
    a.f64[0] = 0.0; b.f64[0] = INFINITY;
    a.f64[1] = -0.0; b.f64[1] = 2.0;
    a.f64[2] = DBL_MAX; b.f64[2] = 2.0;
    a.f64[3] = DBL_MIN; b.f64[3] = 0.5;
    for (int i = 4; i < 8; i++) {
        a.f64[i] = (double)(i + 1) * 1.5;
        b.f64[i] = (double)(i + 2) * 0.5;
    }
    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vmulpd %%zmm1, %%zmm0, %%zmm2\n\tvmovapd %%zmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(IS_QNAN(out.f64[0]), "VMULPD 0*Inf");
    TEST_ASSERT(out.f64[1] == 0.0 && signbit(out.f64[1]), "VMULPD signed zero");
    TEST_ASSERT(isinf(out.f64[2]) && !signbit(out.f64[2]), "VMULPD overflow");
    TEST_ASSERT(out.f64[3] == DBL_MIN / 2.0, "VMULPD subnormal");
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(out.f64[i] == a.f64[i] * b.f64[i], "VMULPD lane %d", i);
    TEST_END();
}
