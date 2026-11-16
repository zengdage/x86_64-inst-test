/* AVX-512F scalar-double mantissa normalization. */
#include "../include/avx512_test.h"
#include <math.h>

int main(void) {
    TEST_START("AVX-512F VGETMANTSD");
    if (!CHECK_AVX512F()) return 0;
    xmm_t src1, src2, out;
    src1.u64[1] = UINT64_C(0x0123456789abcdef);
    const double values[] = {100.0, 0.75, 8.0, -6.0, 0x1p-1074};
    const double expected[] = {1.5625, 1.5, 1.0, -1.5, 1.0};
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        src1.f64[0] = 500.0 + i; src2.f64[0] = values[i];
        __asm__ volatile("vmovapd %1, %%xmm0\n\tvmovapd %2, %%xmm1\n\t"
                         "vgetmantsd $0, %%xmm1, %%xmm0, %%xmm2\n\tvmovapd %%xmm2, %0"
                         : "=m"(out) : "m"(src1), "m"(src2)
                         : "xmm0", "xmm1", "xmm2");
        TEST_ASSERT(out.f64[0] == expected[i], "VGETMANTSD case %u", i);
        TEST_ASSERT(out.u64[1] == src1.u64[1], "VGETMANTSD upper qword case %u", i);
    }
    src2.u64[0] = UINT64_C(0x7ff8000000004321);
    __asm__ volatile("vmovapd %1, %%xmm0\n\tvmovapd %2, %%xmm1\n\t"
                     "vgetmantsd $0, %%xmm1, %%xmm0, %%xmm2\n\tvmovapd %%xmm2, %0"
                     : "=m"(out) : "m"(src1), "m"(src2)
                     : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(IS_QNAN(out.f64[0]), "VGETMANTSD QNaN");
    TEST_END();
}
