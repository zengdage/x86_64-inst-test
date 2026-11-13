/* AVX-512_4FMAPS four-source fused multiply-add instructions. */
#include "avx512_test.h"

int main(void) {
    TEST_START("AVX-512_4FMAPS V4FMADDPS/V4FNMADDPS");
    if (!CHECK_AVX5124FMAPS()) return 0;
    zmm_test_t acc, s0, s1, s2, s3, add, neg;
    float coeff[4] __attribute__((aligned(16))) = {2.0f, -3.0f, 4.0f, -5.0f};
    for (int i = 0; i < 16; i++) {
        acc.f32[i] = (float)i;
        s0.f32[i] = (float)(i + 1);
        s1.f32[i] = 2.0f;
        s2.f32[i] = -1.0f;
        s3.f32[i] = 0.5f;
    }
    __asm__ volatile("vmovaps %2, %%zmm0\n\tvmovaps %3, %%zmm8\n\t"
                     "vmovaps %4, %%zmm9\n\tvmovaps %5, %%zmm10\n\t"
                     "vmovaps %6, %%zmm11\n\tv4fmaddps %7, %%zmm8, %%zmm0\n\t"
                     "vmovaps %%zmm0, %0\n\tvmovaps %2, %%zmm0\n\t"
                     "v4fnmaddps %7, %%zmm8, %%zmm0\n\tvmovaps %%zmm0, %1"
                     : "=m"(add), "=m"(neg)
                     : "m"(acc), "m"(s0), "m"(s1), "m"(s2), "m"(s3), "m"(coeff)
                     : "zmm0", "zmm8", "zmm9", "zmm10", "zmm11");
    for (int i = 0; i < 16; i++) {
        float dot = s0.f32[i] * coeff[0] + s1.f32[i] * coeff[1] +
                    s2.f32[i] * coeff[2] + s3.f32[i] * coeff[3];
        TEST_ASSERT(add.f32[i] == acc.f32[i] + dot, "V4FMADDPS lane %d", i);
        TEST_ASSERT(neg.f32[i] == acc.f32[i] - dot, "V4FNMADDPS lane %d", i);
    }
    TEST_END();
}
