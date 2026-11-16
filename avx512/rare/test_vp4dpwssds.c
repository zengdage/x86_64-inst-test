/* TARGET: VP4DPWSSDS */
#include "../include/avx512_test.h"
#include <limits.h>

int main(void) {
    TEST_START("VP4DPWSSDS");
    if (!CHECK_AVX5124VNNIW()) return 0;
    zmm_test_t acc, s0, s1, s2, s3, out;
    int16_t coeff[8] __attribute__((aligned(16))) = {
        INT16_MAX, INT16_MAX, INT16_MAX, INT16_MAX,
        INT16_MAX, INT16_MAX, INT16_MAX, INT16_MAX
    };
    for (int i = 0; i < 16; i++) acc.i32[i] = (i & 1) ? INT32_MAX : INT32_MIN;
    for (int i = 0; i < 32; i++) {
        int16_t value = (i & 2) ? INT16_MIN : INT16_MAX;
        s0.i16[i] = value;
        s1.i16[i] = value;
        s2.i16[i] = value;
        s3.i16[i] = value;
    }
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm8\n\t"
                     "vmovdqa64 %3, %%zmm9\n\tvmovdqa64 %4, %%zmm10\n\t"
                     "vmovdqa64 %5, %%zmm11\n\tvp4dpwssds %6, %%zmm8, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(acc), "m"(s0), "m"(s1), "m"(s2), "m"(s3), "m"(coeff)
                     : "zmm0", "zmm8", "zmm9", "zmm10", "zmm11");
    const zmm_test_t *sources[4] = {&s0, &s1, &s2, &s3};
    for (int lane = 0; lane < 16; lane++) {
        int64_t sum = acc.i32[lane];
        for (int s = 0; s < 4; s++)
            sum += (int64_t)sources[s]->i16[lane * 2] * coeff[s * 2] +
                   (int64_t)sources[s]->i16[lane * 2 + 1] * coeff[s * 2 + 1];
        int32_t expected = sum > INT32_MAX ? INT32_MAX :
                           sum < INT32_MIN ? INT32_MIN : (int32_t)sum;
        TEST_ASSERT(out.i32[lane] == expected, "VP4DPWSSDS lane %d", lane);
    }
    TEST_END();
}
