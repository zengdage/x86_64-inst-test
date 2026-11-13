/* AVX-512_4VNNIW four-source word dot products. */
#include "avx512_test.h"
#include <limits.h>

int main(void) {
    TEST_START("AVX-512_4VNNIW VP4DPWSSD/VP4DPWSSDS");
    if (!CHECK_AVX5124VNNIW()) return 0;
    zmm_test_t acc, s0, s1, s2, s3, normal, saturated;
    int16_t coeff[8] __attribute__((aligned(16))) = {1, -2, 3, -4, 5, -6, 7, -8};
    for (int i = 0; i < 16; i++) acc.i32[i] = i - 8;
    for (int i = 0; i < 32; i++) {
        s0.i16[i] = (int16_t)(i + 1); s1.i16[i] = (int16_t)(2 * i - 20);
        s2.i16[i] = (int16_t)(30 - i); s3.i16[i] = (int16_t)(i * 3 - 40);
    }
    __asm__ volatile("vmovdqa32 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm8\n\t"
                     "vmovdqa64 %4, %%zmm9\n\tvmovdqa64 %5, %%zmm10\n\t"
                     "vmovdqa64 %6, %%zmm11\n\tvp4dpwssd %7, %%zmm8, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %0\n\tvmovdqa32 %2, %%zmm0\n\t"
                     "vp4dpwssds %7, %%zmm8, %%zmm0\n\tvmovdqa32 %%zmm0, %1"
                     : "=m"(normal), "=m"(saturated)
                     : "m"(acc), "m"(s0), "m"(s1), "m"(s2), "m"(s3), "m"(coeff)
                     : "zmm0", "zmm8", "zmm9", "zmm10", "zmm11");
    const zmm_test_t *sources[4] = {&s0, &s1, &s2, &s3};
    for (int lane = 0; lane < 16; lane++) {
        int64_t sum = acc.i32[lane];
        for (int s = 0; s < 4; s++)
            sum += (int64_t)sources[s]->i16[lane * 2] * coeff[s * 2] +
                   (int64_t)sources[s]->i16[lane * 2 + 1] * coeff[s * 2 + 1];
        int32_t sat = sum > INT32_MAX ? INT32_MAX : sum < INT32_MIN ? INT32_MIN : (int32_t)sum;
        TEST_ASSERT(normal.u32[lane] == (uint32_t)sum, "VP4DPWSSD lane %d", lane);
        TEST_ASSERT(saturated.i32[lane] == sat, "VP4DPWSSDS lane %d", lane);
    }
    TEST_END();
}
