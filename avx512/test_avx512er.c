/* AVX-512ER exponential, reciprocal, and reciprocal-square-root estimates. */
#include "avx512_test.h"
#include <math.h>

int main(void) {
    TEST_START("AVX-512ER VEXP2/VRCP28/VRSQRT28");
    if (!CHECK_AVX512ER()) return 0;
    zmm_test_t src, exp2out, rcp, rsqrt;
    for (int i = 0; i < 16; i++) src.f32[i] = (float)((i & 7) + 1);
    src.f32[0] = 0.0f;
    src.f32[1] = 1.0f;
    src.f32[2] = 4.0f;
    src.f32[3] = INFINITY;
    __asm__ volatile("vmovaps %3, %%zmm0\n\tvexp2ps %%zmm0, %%zmm1\n\t"
                     "vrcp28ps %%zmm0, %%zmm2\n\tvrsqrt28ps %%zmm0, %%zmm3\n\t"
                     "vmovaps %%zmm1, %0\n\tvmovaps %%zmm2, %1\n\tvmovaps %%zmm3, %2"
                     : "=m"(exp2out), "=m"(rcp), "=m"(rsqrt) : "m"(src)
                     : "zmm0", "zmm1", "zmm2", "zmm3");
    for (int i = 0; i < 16; i++) {
        if (isfinite(src.f32[i]))
            TEST_ASSERT(fabsf(exp2out.f32[i] - exp2f(src.f32[i])) <= exp2f(src.f32[i]) * 0x1p-22f,
                        "VEXP2PS lane %d", i);
        if (isfinite(src.f32[i]) && src.f32[i] > 0.0f) {
            float er = 1.0f / src.f32[i];
            float es = 1.0f / sqrtf(src.f32[i]);
            TEST_ASSERT(fabsf(rcp.f32[i] - er) <= er * 0x1p-27f, "VRCP28PS lane %d", i);
            TEST_ASSERT(fabsf(rsqrt.f32[i] - es) <= es * 0x1p-27f, "VRSQRT28PS lane %d", i);
        }
    }
    TEST_ASSERT(isinf(rcp.f32[0]), "VRCP28PS zero -> infinity");
    TEST_ASSERT(exp2out.f32[3] == INFINITY && rcp.f32[3] == 0.0f,
                "AVX-512ER infinity boundaries");
    TEST_END();
}
