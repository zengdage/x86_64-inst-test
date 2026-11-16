/* AVX-512F scalar-double rounding with immediate controls. */
#include "../include/avx512_test.h"
#include <math.h>

#define RUN(IMM) do { \
    __asm__ volatile("vmovapd %1, %%xmm0\n\tvmovapd %2, %%xmm1\n\t" \
                     "vrndscalesd $" #IMM ", %%xmm1, %%xmm0, %%xmm2\n\t" \
                     "vmovapd %%xmm2, %0" \
                     : "=m"(out) : "m"(src1), "m"(src2) : "xmm0", "xmm1", "xmm2"); \
} while (0)

int main(void) {
    TEST_START("AVX-512F VRNDSCALESD");
    if (!CHECK_AVX512F()) return 0;
    xmm_t src1, src2, out;
    src1.u64[1] = UINT64_C(0xa1a2a3a4a5a6a7a8);
    src2.f64[0] = 2.5;
    RUN(0); TEST_ASSERT(out.f64[0] == 2.0, "VRNDSCALESD nearest-even");
    RUN(1); TEST_ASSERT(out.f64[0] == 2.0, "VRNDSCALESD down");
    RUN(2); TEST_ASSERT(out.f64[0] == 3.0, "VRNDSCALESD up");
    RUN(3); TEST_ASSERT(out.f64[0] == 2.0, "VRNDSCALESD truncate");
    TEST_ASSERT(out.u64[1] == src1.u64[1], "VRNDSCALESD upper qword");
    src2.f64[0] = -2.5;
    RUN(1); TEST_ASSERT(out.f64[0] == -3.0, "VRNDSCALESD negative down");
    RUN(2); TEST_ASSERT(out.f64[0] == -2.0, "VRNDSCALESD negative up");
    src2.f64[0] = -0.0; RUN(0);
    TEST_ASSERT(out.f64[0] == 0.0 && signbit(out.f64[0]), "VRNDSCALESD negative zero");
    src2.f64[0] = INFINITY; RUN(0);
    TEST_ASSERT(isinf(out.f64[0]) && !signbit(out.f64[0]), "VRNDSCALESD +Inf");
    TEST_END();
}

#undef RUN
