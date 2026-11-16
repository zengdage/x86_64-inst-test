/* TARGET: VCVTTPD2DQ */
#include "../include/avx512_test.h"
#include <math.h>

int main(void) {
    TEST_START("VCVTTPD2DQ");
    if (!CHECK_AVX512F()) return 0;

    zmm_test_t input;
    const double values[8] = {
        0.0, 1.9, -1.9, 2147483647.0,
        -2147483648.0, NAN, INFINITY, 42.75
    };
    const uint32_t expected[8] = {
        0, 1, UINT32_MAX, UINT32_C(0x7fffffff),
        UINT32_C(0x80000000), UINT32_C(0x80000000),
        UINT32_C(0x80000000), 42
    };
    for (int i = 0; i < 8; i++) input.f64[i] = values[i];

    uint32_t out[8] __attribute__((aligned(32)));
    __asm__ volatile("vmovapd %1, %%zmm0\n\t"
                     "vcvttpd2dq %%zmm0, %%ymm1\n\t"
                     "vmovdqa32 %%ymm1, %0"
                     : "=m"(out) : "m"(input) : "zmm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out[i] == expected[i],
                    "VCVTTPD2DQ lane %d: %08x != %08x",
                    i, out[i], expected[i]);

    uint32_t mask = UINT32_C(0x81);
    __asm__ volatile("kmovb %2, %%k1\n\tvmovapd %1, %%zmm0\n\t"
                     "vcvttpd2dq %%zmm0, %%ymm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%ymm1, %0"
                     : "=m"(out) : "m"(input), "r"(mask)
                     : "zmm0", "ymm1", "k1");
    for (int i = 0; i < 8; i++) {
        uint32_t value = (i == 0 || i == 7) ? expected[i] : 0;
        TEST_ASSERT(out[i] == value, "VCVTTPD2DQ zero mask lane %d", i);
    }
    TEST_END();
}
