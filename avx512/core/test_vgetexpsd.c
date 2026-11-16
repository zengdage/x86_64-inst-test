/* AVX-512F scalar-double exponent extraction. */
#include "../include/avx512_test.h"
#include <math.h>

static void run(xmm_t *out, const xmm_t *src1, const xmm_t *src2, uint32_t mask, int zero) {
    if (zero)
        __asm__ volatile("kmovb %3, %%k1\n\tvmovapd %1, %%xmm0\n\tvmovapd %2, %%xmm1\n\t"
                         "vgetexpsd %%xmm1, %%xmm0, %%xmm2%{%%k1%}%{z%}\n\tvmovapd %%xmm2, %0"
                         : "=m"(*out) : "m"(*src1), "m"(*src2), "r"(mask)
                         : "xmm0", "xmm1", "xmm2", "k1");
    else
        __asm__ volatile("kmovb %3, %%k1\n\tvmovapd %1, %%xmm0\n\tvmovapd %2, %%xmm1\n\t"
                         "vgetexpsd %%xmm1, %%xmm0, %%xmm0%{%%k1%}\n\tvmovapd %%xmm0, %0"
                         : "=m"(*out) : "m"(*src1), "m"(*src2), "r"(mask)
                         : "xmm0", "xmm1", "k1");
}

int main(void) {
    TEST_START("AVX-512F VGETEXPSD");
    if (!CHECK_AVX512F()) return 0;
    xmm_t src1, src2, out;
    src1.u64[1] = UINT64_C(0x1122334455667788);
    const double values[] = {8.0, 0.5, 1.0, 100.0, 0x1p-1074, -6.0};
    const double expected[] = {3.0, -1.0, 0.0, 6.0, -1074.0, 2.0};
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        src1.f64[0] = -100.0 - i; src2.f64[0] = values[i];
        run(&out, &src1, &src2, 1, 0);
        TEST_ASSERT(out.f64[0] == expected[i], "VGETEXPSD case %u", i);
        TEST_ASSERT(out.u64[1] == src1.u64[1], "VGETEXPSD upper qword case %u", i);
    }
    src2.f64[0] = 0.0; run(&out, &src1, &src2, 1, 0);
    TEST_ASSERT(isinf(out.f64[0]) && signbit(out.f64[0]), "VGETEXPSD zero -> -Inf");
    src2.f64[0] = INFINITY; run(&out, &src1, &src2, 1, 0);
    TEST_ASSERT(isinf(out.f64[0]) && !signbit(out.f64[0]), "VGETEXPSD +Inf");
    src1.f64[0] = -123.0; src2.f64[0] = 8.0; run(&out, &src1, &src2, 0, 0);
    TEST_ASSERT(out.f64[0] == src1.f64[0], "VGETEXPSD merge mask");
    run(&out, &src1, &src2, 0, 1);
    TEST_ASSERT(out.u64[0] == 0 && out.u64[1] == src1.u64[1], "VGETEXPSD zero mask");
    TEST_END();
}
