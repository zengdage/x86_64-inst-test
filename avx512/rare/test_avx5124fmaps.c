/* AVX-512_4FMAPS four-source fused multiply-add instructions. */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("AVX-512_4FMAPS packed and scalar forms");
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

    xmm_t scalar_acc, scalar_s0, scalar_s1, scalar_s2, scalar_s3;
    xmm_t scalar_add, scalar_neg;
    for (int i = 0; i < 4; i++) {
        scalar_acc.f32[i] = 10.0f + i;
        scalar_s0.f32[i] = 1.0f + i;
        scalar_s1.f32[i] = 2.0f + i;
        scalar_s2.f32[i] = -1.0f - i;
        scalar_s3.f32[i] = 0.5f + i;
    }
    __asm__ volatile(
        "vmovaps %2, %%xmm0\n\tvmovaps %3, %%xmm8\n\t"
        "vmovaps %4, %%xmm9\n\tvmovaps %5, %%xmm10\n\t"
        "vmovaps %6, %%xmm11\n\t"
        "v4fmaddss %7, %%xmm8, %%xmm0\n\tvmovaps %%xmm0, %0\n\t"
        "vmovaps %2, %%xmm0\n\tv4fnmaddss %7, %%xmm8, %%xmm0\n\t"
        "vmovaps %%xmm0, %1"
        : "=m"(scalar_add), "=m"(scalar_neg)
        : "m"(scalar_acc), "m"(scalar_s0), "m"(scalar_s1),
          "m"(scalar_s2), "m"(scalar_s3), "m"(coeff)
        : "xmm0", "xmm8", "xmm9", "xmm10", "xmm11");
    float scalar_dot = scalar_s0.f32[0] * coeff[0] +
                       scalar_s1.f32[0] * coeff[1] +
                       scalar_s2.f32[0] * coeff[2] +
                       scalar_s3.f32[0] * coeff[3];
    TEST_ASSERT(scalar_add.f32[0] == scalar_acc.f32[0] + scalar_dot,
                "V4FMADDSS scalar result");
    TEST_ASSERT(scalar_neg.f32[0] == scalar_acc.f32[0] - scalar_dot,
                "V4FNMADDSS scalar result");
    for (int i = 1; i < 4; i++) {
        TEST_ASSERT(scalar_add.u32[i] == scalar_acc.u32[i],
                    "V4FMADDSS preserves destination lane %d", i);
        TEST_ASSERT(scalar_neg.u32[i] == scalar_acc.u32[i],
                    "V4FNMADDSS preserves destination lane %d", i);
    }
    TEST_END();
}
