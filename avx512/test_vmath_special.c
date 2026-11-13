/* AVX-512 special math, rounding, range, exponent/mantissa, and FP classes. */
#include "avx512_test.h"
#include <float.h>
#include <math.h>

static void test_sqrt_and_reciprocal(void) {
    zmm_test_t src, sqrt_out, rcp, rsqrt;
    const float values[16] = {
        0.0f, -0.0f, 1.0f, 4.0f, 9.0f, 16.0f, INFINITY, NAN,
        -1.0f, FLT_MIN, 0x1p-148f, 0.25f, 2.0f, 64.0f, 100.0f, 256.0f
    };
    memcpy(src.f32, values, sizeof(values));
    __asm__ volatile(
        "vmovaps %3, %%zmm0\n\tvsqrtps %%zmm0, %%zmm1\n\t"
        "vrcp14ps %%zmm0, %%zmm2\n\tvrsqrt14ps %%zmm0, %%zmm3\n\t"
        "vmovaps %%zmm1, %0\n\tvmovaps %%zmm2, %1\n\tvmovaps %%zmm3, %2"
        : "=m"(sqrt_out), "=m"(rcp), "=m"(rsqrt) : "m"(src)
        : "zmm0", "zmm1", "zmm2", "zmm3");
    TEST_ASSERT(sqrt_out.f32[0] == 0.0f && !signbit(sqrt_out.f32[0]), "VSQRTPS +0 sign");
    TEST_ASSERT(sqrt_out.f32[1] == 0.0f && signbit(sqrt_out.f32[1]), "VSQRTPS -0 sign");
    TEST_ASSERT(sqrt_out.f32[3] == 2.0f && sqrt_out.f32[4] == 3.0f,
                "VSQRTPS exact square roots");
    TEST_ASSERT(isinf(sqrt_out.f32[6]) && !signbit(sqrt_out.f32[6]), "VSQRTPS +Inf");
    TEST_ASSERT(IS_QNAN(sqrt_out.f32[7]) && IS_QNAN(sqrt_out.f32[8]),
                "VSQRTPS QNaN and negative input");

    TEST_ASSERT(isinf(rcp.f32[0]) && !signbit(rcp.f32[0]), "VRCP14PS reciprocal +0");
    TEST_ASSERT(isinf(rcp.f32[1]) && signbit(rcp.f32[1]), "VRCP14PS reciprocal -0");
    TEST_ASSERT(rcp.f32[6] == 0.0f && !signbit(rcp.f32[6]), "VRCP14PS reciprocal +Inf");
    TEST_ASSERT(IS_QNAN(rcp.f32[7]), "VRCP14PS QNaN propagation");
    for (int i = 2; i < 16; i++) {
        if (isfinite(src.f32[i]) && src.f32[i] > 0.0f) {
            float exact = 1.0f / src.f32[i];
            if (!isfinite(exact)) continue;
            float relative = fabsf((rcp.f32[i] - exact) / exact);
            TEST_ASSERT(relative <= 0x1p-13f, "VRCP14PS lane %d relative error %g", i, relative);
        }
    }

    TEST_ASSERT(isinf(rsqrt.f32[0]) && !signbit(rsqrt.f32[0]), "VRSQRT14PS +0");
    TEST_ASSERT(rsqrt.f32[6] == 0.0f && !signbit(rsqrt.f32[6]), "VRSQRT14PS +Inf");
    TEST_ASSERT(IS_QNAN(rsqrt.f32[7]) && IS_QNAN(rsqrt.f32[8]),
                "VRSQRT14PS QNaN and negative input");
    for (int i = 2; i < 16; i++) {
        if (isfinite(src.f32[i]) && src.f32[i] > 0.0f) {
            float exact = 1.0f / sqrtf(src.f32[i]);
            float relative = fabsf((rsqrt.f32[i] - exact) / exact);
            TEST_ASSERT(relative <= 0x1p-13f, "VRSQRT14PS lane %d relative error %g", i, relative);
        }
    }
}

static void test_getexp_getmant(void) {
    zmm_test_t src, exponent, mantissa;
    const float values[16] = {
        1.0f, 2.0f, 0.5f, 8.0f, 12.0f, 0.75f, FLT_MIN, 0x1p-149f,
        INFINITY, -INFINITY, 0.0f, -0.0f, NAN, -6.0f, 3.0f, 0x1.8p10f
    };
    memcpy(src.f32, values, sizeof(values));
    __asm__ volatile(
        "vmovaps %2, %%zmm0\n\tvgetexpps %%zmm0, %%zmm1\n\t"
        "vgetmantps $0, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm1, %0\n\tvmovaps %%zmm2, %1"
        : "=m"(exponent), "=m"(mantissa) : "m"(src)
        : "zmm0", "zmm1", "zmm2");
    const float expected_exp[8] = {0, 1, -1, 3, 3, -1, -126, -149};
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(exponent.f32[i] == expected_exp[i], "VGETEXPPS lane %d", i);
    TEST_ASSERT(isinf(exponent.f32[8]) && !signbit(exponent.f32[8]), "VGETEXPPS +Inf");
    TEST_ASSERT(isinf(exponent.f32[10]) && signbit(exponent.f32[10]), "VGETEXPPS zero -> -Inf");
    TEST_ASSERT(IS_QNAN(exponent.f32[12]), "VGETEXPPS QNaN");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(mantissa.f32[i] >= 1.0f && mantissa.f32[i] < 2.0f,
                    "VGETMANTPS normalized lane %d: %a", i, mantissa.f32[i]);
        TEST_ASSERT(ldexpf(mantissa.f32[i], (int)exponent.f32[i]) == src.f32[i],
                    "VGETMANTPS reconstruct lane %d", i);
    }
    TEST_ASSERT(mantissa.f32[13] == -1.5f, "VGETMANTPS preserves negative sign");
}

static void test_round_reduce_range(void) {
    zmm_test_t src, other, rounded, reduced, ranged;
    const float values[16] = {
        0.5f, 1.5f, 2.5f, 3.5f, -0.5f, -1.5f, -2.5f, -3.5f,
        1.25f, -1.25f, 15.75f, -15.75f, 0.0f, -0.0f, INFINITY, NAN
    };
    memcpy(src.f32, values, sizeof(values));
    for (int i = 0; i < 16; i++) other.f32[i] = (float)(i - 4);
    __asm__ volatile(
        "vmovaps %3, %%zmm0\n\tvmovaps %4, %%zmm1\n\t"
        "vrndscaleps $0, %%zmm0, %%zmm2\n\t"
        "vreduceps $0, %%zmm0, %%zmm3\n\t"
        "vrangeps $0, %%zmm1, %%zmm0, %%zmm4\n\t"
        "vmovaps %%zmm2, %0\n\tvmovaps %%zmm3, %1\n\tvmovaps %%zmm4, %2"
        : "=m"(rounded), "=m"(reduced), "=m"(ranged) : "m"(src), "m"(other)
        : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4");
    const float nearest[8] = {0, 2, 2, 4, -0.0f, -2, -2, -4};
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(rounded.f32[i] == nearest[i], "VRNDSCALEPS ties-to-even lane %d", i);
        TEST_ASSERT(reduced.f32[i] == src.f32[i] - nearbyintf(src.f32[i]),
                    "VREDUCEPS lane %d", i);
    }
    TEST_ASSERT(rounded.f32[13] == 0.0f && signbit(rounded.f32[13]), "VRNDSCALEPS -0 sign");
    TEST_ASSERT(isinf(rounded.f32[14]) && IS_QNAN(rounded.f32[15]),
                "VRNDSCALEPS Inf/QNaN");
    for (int i = 0; i < 14; i++) {
        float expected = copysignf(fabsf(fminf(src.f32[i], other.f32[i])), src.f32[i]);
        TEST_ASSERT(ranged.f32[i] == expected,
                    "VRANGEPS minimum lane %d", i);
    }
}

#define RUN_FPCLASS(IMM, EXPECTED) do { \
    uint32_t mask; \
    __asm__ volatile("vmovaps %1, %%zmm0\n\t" \
                     "vfpclassps $" #IMM ", %%zmm0, %%k1\n\tkmovw %%k1, %0" \
                     : "=r"(mask) : "m"(src) : "zmm0", "k1"); \
    TEST_ASSERT((mask & 0xffffU) == (EXPECTED), \
                "VFPCLASSPS imm 0x%02x: 0x%04x", IMM, mask); \
} while (0)

static void test_fpclass(void) {
    zmm_test_t src = {0};
    src.u32[0] = UINT32_C(0x7fc12345); /* QNaN */
    src.u32[1] = UINT32_C(0x00000000); /* +0 */
    src.u32[2] = UINT32_C(0x80000000); /* -0 */
    src.u32[3] = UINT32_C(0x7f800000); /* +Inf */
    src.u32[4] = UINT32_C(0xff800000); /* -Inf */
    src.u32[5] = UINT32_C(0x00000001); /* positive denormal */
    src.f32[6] = -1.0f;
    src.u32[7] = UINT32_C(0x7f800001); /* SNaN */
    for (int i = 8; i < 16; i++) src.f32[i] = (float)(i + 1);
    RUN_FPCLASS(0x01, 0x0001U);
    RUN_FPCLASS(0x02, 0x0002U);
    RUN_FPCLASS(0x04, 0x0004U);
    RUN_FPCLASS(0x08, 0x0008U);
    RUN_FPCLASS(0x10, 0x0010U);
    RUN_FPCLASS(0x20, 0x0020U);
    RUN_FPCLASS(0x40, 0x0040U);
    RUN_FPCLASS(0x80, 0x0080U);
    RUN_FPCLASS(0xff, 0x00ffU);
}

#undef RUN_FPCLASS

int main(void) {
    TEST_START("AVX-512 sqrt/reciprocal/exponent/mantissa/round/range/class");
    if (!CHECK_AVX512F()) return 0;
    test_sqrt_and_reciprocal();
    test_getexp_getmant();
    test_round_reduce_range();
    test_fpclass();
    TEST_END();
}
