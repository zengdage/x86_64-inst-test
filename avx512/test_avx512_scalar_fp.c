/*
 * EVEX-encoded AVX-512 scalar floating point instructions.
 *
 * Covers the scalar single and double precision forms that only exist with
 * an EVEX prefix: VADDSS/VADDSD, VSUBSS/VSUBSD, VMULSS/VMULSD, VDIVSS/VDIVSD,
 * VMINSS/VMINSD, VMAXSS/VMAXSD, VSQRTSS/VSQRTSD, VROUNDSS/VROUNDSD,
 * VRANGESS/VRANGESD, VREDUCESS/VREDUCESD, VGETEXPSS/VGETEXPSD,
 * VGETMANTSS/VGETMANTSD, VRNDSCALESS/VRNDSCALESD, VSCALEFSS/VSCALEFSD,
 * VRCP14SS/VRCP14SD, VRSQRT14SS/VRSQqrt14SD, VFPCLASSSS/VFPCLASSSD and the
 * scalar conversions VCVTSD2SS, VCVTSS2SD, VCVTSD2USI, VCVTTSD2USI,
 * VCVTUSI2SD and VCVTUSI2SS.
 *
 * Operand conventions of the EVEX scalar encodings used below:
 *   - the first AT&T operand is the memory capable source slot,
 *   - the second AT&T operand is the register source slot,
 *   - the third AT&T operand is the destination.
 * Two input instructions combine the register source (src1) with the memory
 * capable source (src2); single input instructions read the memory capable
 * source.  Bits 127:32 of the destination always receive the contents of the
 * register source, independently of the mask.
 */
#include "avx512_test.h"
#include <float.h>
#include <math.h>

#define SRC1_UPPER_PATTERN UINT32_C(0xaaaaaaaa)
#define DST_UPPER_PATTERN  UINT32_C(0xcccccccc)

/*
 * RUN_SS(ins, out)
 *
 * Executes an EVEX single precision instruction with the destination masked
 * by k1.  The destination register is preloaded with DST_UPPER_PATTERN so
 * that the merge and zero masking behaviour can be observed.
 */
#define RUN_SS(ins, out)                                                        \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovaps %1, %%xmm0\n\t"                                             \
            "vmovaps %2, %%xmm1\n\t"                                             \
            "vmovaps %4, %%xmm2\n\t"                                             \
            ins " %%xmm1, %%xmm0, %%xmm2%{%%k1%}\n\t"                           \
            "vmovaps %%xmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "xmm0", "xmm1", "xmm2", "k1");                                     \
    } while (0)

/* Same, but with zero masking. */
#define RUN_SS_ZERO(ins, out)                                                   \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovaps %1, %%xmm0\n\t"                                             \
            "vmovaps %2, %%xmm1\n\t"                                             \
            "vmovaps %4, %%xmm2\n\t"                                             \
            ins " %%xmm1, %%xmm0, %%xmm2%{%%k1%}%{z%}\n\t"                      \
            "vmovaps %%xmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "xmm0", "xmm1", "xmm2", "k1");                                     \
    } while (0)

/* Single precision instruction with an immediate operand. */
#define RUN_SS_IMM(ins, imm, out)                                               \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovaps %1, %%xmm0\n\t"                                             \
            "vmovaps %2, %%xmm1\n\t"                                             \
            "vmovaps %4, %%xmm2\n\t"                                             \
            ins " $" imm ", %%xmm1, %%xmm0, %%xmm2%{%%k1%}\n\t"                  \
            "vmovaps %%xmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "xmm0", "xmm1", "xmm2", "k1");                                     \
    } while (0)

#define RUN_SD(ins, out)                                                        \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovapd %1, %%xmm0\n\t"                                             \
            "vmovapd %2, %%xmm1\n\t"                                             \
            "vmovapd %4, %%xmm2\n\t"                                             \
            ins " %%xmm1, %%xmm0, %%xmm2%{%%k1%}\n\t"                           \
            "vmovapd %%xmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "xmm0", "xmm1", "xmm2", "k1");                                     \
    } while (0)

#define RUN_SD_IMM(ins, imm, out)                                               \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovapd %1, %%xmm0\n\t"                                             \
            "vmovapd %2, %%xmm1\n\t"                                             \
            "vmovapd %4, %%xmm2\n\t"                                             \
            ins " $" imm ", %%xmm1, %%xmm0, %%xmm2%{%%k1%}\n\t"                  \
            "vmovapd %%xmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "xmm0", "xmm1", "xmm2", "k1");                                     \
    } while (0)

static xmm_t src1, src2, dst_init, out;

static void init_operands(void) {
    for (int i = 0; i < 4; i++) {
        src1.u32[i] = SRC1_UPPER_PATTERN;
        src2.u32[i] = SRC1_UPPER_PATTERN ^ UINT32_C(0x55555555);
        dst_init.u32[i] = DST_UPPER_PATTERN;
    }
}

static void test_scalar_add_sub_mul_div(void) {
    static const float lhs_values[4] = {3.5f, -2.25f, 0.0f, 7.5f};
    static const float rhs_values[4] = {1.25f, 4.0f, -2.0f, -2.5f};
    uint32_t mask = 0xffffU;

    init_operands();

    for (int i = 0; i < 4; i++) {
        src1.f32[0] = lhs_values[i];
        src2.f32[0] = rhs_values[i];

        RUN_SS("vaddss", out);
        TEST_ASSERT(out.f32[0] == src1.f32[0] + src2.f32[0], "VADDSS case %d", i);
        TEST_ASSERT(out.u32[1] == SRC1_UPPER_PATTERN,
                    "VADDSS copies the upper bits of the register source case %d", i);

        RUN_SS("vsubss", out);
        TEST_ASSERT(out.f32[0] == src1.f32[0] - src2.f32[0], "VSUBSS case %d", i);

        RUN_SS("vmulss", out);
        TEST_ASSERT(out.f32[0] == src1.f32[0] * src2.f32[0], "VMULSS case %d", i);

        RUN_SS("vdivss", out);
        TEST_ASSERT(out.f32[0] == src1.f32[0] / src2.f32[0], "VDIVSS case %d", i);

        RUN_SS("vminss", out);
        TEST_ASSERT(out.f32[0] == fminf(src1.f32[0], src2.f32[0]), "VMINSS case %d", i);

        RUN_SS("vmaxss", out);
        TEST_ASSERT(out.f32[0] == fmaxf(src1.f32[0], src2.f32[0]), "VMAXSS case %d", i);
    }

    /* Equal values, including zeros of different sign, select the second
     * source, so both VMINSS and VMAXSS return -0.0 here. */
    src1.f32[0] = 0.0f;
    src2.f32[0] = -0.0f;
    RUN_SS("vminss", out);
    TEST_ASSERT(out.f32[0] == 0.0f && signbit(out.f32[0]),
                "VMINSS keeps -0.0 for mixed zeros");
    RUN_SS("vmaxss", out);
    TEST_ASSERT(out.f32[0] == 0.0f && signbit(out.f32[0]),
                "VMAXSS keeps -0.0 for mixed zeros");

    /* Infinities and NaN propagation. */
    src1.f32[0] = INFINITY;
    src2.f32[0] = 1.0f;
    RUN_SS("vdivss", out);
    TEST_ASSERT(isinf(out.f32[0]) && out.f32[0] > 0.0f, "VDIVSS +Inf operand");
    RUN_SS("vsubss", out);
    TEST_ASSERT(isinf(out.f32[0]) && out.f32[0] > 0.0f, "VSUBSS +Inf operand");

    src1.f32[0] = NAN;
    src2.f32[0] = 1.0f;
    RUN_SS("vaddss", out);
    TEST_ASSERT(IS_QNAN(out.f32[0]), "VADDSS propagates QNaN");

    /* Dividing zero by zero produces the default NaN. */
    src1.f32[0] = 0.0f;
    src2.f32[0] = 0.0f;
    RUN_SS("vdivss", out);
    TEST_ASSERT(IS_QNAN(out.f32[0]), "VDIVSS of zero by zero is QNaN");

    /* Double precision variants. */
    {
        static const double dl[4] = {3.5, -2.25, 1e308, 0.0};
        static const double dr[4] = {1.25, 4.0, 1e308, -2.0};
        for (int i = 0; i < 4; i++) {
            src1.f64[0] = dl[i];
            src2.f64[0] = dr[i];

            RUN_SD("vaddsd", out);
            TEST_ASSERT(out.f64[0] == dl[i] + dr[i], "VADDSD case %d", i);
            RUN_SD("vsubsd", out);
            TEST_ASSERT(out.f64[0] == dl[i] - dr[i], "VSUBSD case %d", i);
            RUN_SD("vmulsd", out);
            TEST_ASSERT(out.f64[0] == dl[i] * dr[i], "VMULSD case %d", i);
            RUN_SD("vdivsd", out);
            TEST_ASSERT(out.f64[0] == dl[i] / dr[i], "VDIVSD case %d", i);
            RUN_SD("vminsd", out);
            TEST_ASSERT(out.f64[0] == fmin(dl[i], dr[i]), "VMINSD case %d", i);
            RUN_SD("vmaxsd", out);
            TEST_ASSERT(out.f64[0] == fmax(dl[i], dr[i]), "VMAXSD case %d", i);
            TEST_ASSERT(out.u64[1] == UINT64_C(0xaaaaaaaaaaaaaaaa),
                        "VADDSD copies the upper bits of the register source case %d", i);
        }
    }
}

static void test_scalar_masking(void) {
    uint32_t mask;

    init_operands();
    src1.f32[0] = 5.0f;
    src2.f32[0] = 3.0f;

    /* Merge masking with an empty mask keeps the destination element. */
    mask = 0U;
    RUN_SS("vaddss", out);
    TEST_ASSERT(out.u32[0] == DST_UPPER_PATTERN,
                "VADDSS merge mask zero keeps the destination element");
    TEST_ASSERT(out.u32[1] == SRC1_UPPER_PATTERN,
                "VADDSS merge mask zero still copies the upper bits");

    /* Zero masking with an empty mask clears the destination element. */
    RUN_SS_ZERO("vaddss", out);
    TEST_ASSERT(out.u32[0] == 0U,
                "VADDSS zero mask clears the destination element");
    TEST_ASSERT(out.u32[1] == SRC1_UPPER_PATTERN,
                "VADDSS zero mask still copies the upper bits");

    /* A mask that only enables bit 0 performs the operation. */
    mask = 0x1U;
    RUN_SS("vaddss", out);
    TEST_ASSERT(out.f32[0] == 8.0f, "VADDSS masked lane result");
    RUN_SS_ZERO("vaddss", out);
    TEST_ASSERT(out.f32[0] == 8.0f, "VADDSS masked lane zeroing result");
}

static void test_scalar_sqrt_reciprocal(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    src2.f32[0] = 16.0f;
    RUN_SS("vsqrtss", out);
    TEST_ASSERT(out.f32[0] == 4.0f, "VSQRTSS exact square root");
    TEST_ASSERT(out.u32[1] == SRC1_UPPER_PATTERN, "VSQRTSS copies the upper bits");

    src2.f32[0] = -0.0f;
    RUN_SS("vsqrtss", out);
    TEST_ASSERT(out.f32[0] == 0.0f && signbit(out.f32[0]),
                "VSQRTSS of -0.0 keeps the sign");

    src2.f32[0] = INFINITY;
    RUN_SS("vsqrtss", out);
    TEST_ASSERT(isinf(out.f32[0]) && !signbit(out.f32[0]), "VSQRTSS of +Inf");

    src2.f32[0] = -1.0f;
    RUN_SS("vsqrtss", out);
    TEST_ASSERT(IS_QNAN(out.f32[0]), "VSQRTSS of a negative value is QNaN");

    src2.f64[0] = 2.25;
    RUN_SD("vsqrtsd", out);
    TEST_ASSERT(out.f64[0] == 1.5, "VSQRTSD exact square root");

    /* The 14 bit approximations are accurate to at least 2^-14 relative. */
    src2.f32[0] = 4.0f;
    RUN_SS("vrcp14ss", out);
    TEST_ASSERT(fabsf(out.f32[0] - 0.25f) <= 0.25f * 0x1p-14f, "VRCP14SS reciprocal");

    src2.f32[0] = 16.0f;
    RUN_SS("vrsqrt14ss", out);
    TEST_ASSERT(fabsf(out.f32[0] - 0.25f) <= 0.25f * 0x1p-14f,
                "VRSQRT14SS reciprocal root");

    src2.f64[0] = 9.0;
    RUN_SD("vrcp14sd", out);
    TEST_ASSERT(fabs(out.f64[0] - 1.0 / 9.0) <= (1.0 / 9.0) * 0x1p-14,
                "VRCP14SD reciprocal");
    RUN_SD("vrsqrt14sd", out);
    TEST_ASSERT(fabs(out.f64[0] - 1.0 / 3.0) <= (1.0 / 3.0) * 0x1p-14,
                "VRSQRT14SD reciprocal root");
}

static void test_scalar_round(void) {
    init_operands();

    /*
     * Round to nearest even, down, up and toward zero.  The assembler in
     * this toolchain only accepts the unmasked VROUNDSS/VROUNDSD form, so
     * the destination is the third operand and the rounded value is the
     * first operand.
     */
#define RUN_ROUND_SS(imm, out)                                                  \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "vmovaps %1, %%xmm0\n\t"                                             \
            "vroundss $" imm ", %%xmm0, %%xmm1, %%xmm2\n\t"                      \
            "vmovaps %%xmm2, %0"                                                \
            : "=m"(out) : "m"(src2) : "xmm0", "xmm1", "xmm2");                   \
    } while (0)

    src2.f32[0] = 2.5f;
    RUN_ROUND_SS("0", out);
    TEST_ASSERT(out.f32[0] == 2.0f, "VROUNDSS round to nearest even");
    RUN_ROUND_SS("1", out);
    TEST_ASSERT(out.f32[0] == 2.0f, "VROUNDSS round down");
    RUN_ROUND_SS("2", out);
    TEST_ASSERT(out.f32[0] == 3.0f, "VROUNDSS round up");
    RUN_ROUND_SS("3", out);
    TEST_ASSERT(out.f32[0] == 2.0f, "VROUNDSS truncate");

    src2.f32[0] = -2.5f;
    RUN_ROUND_SS("0", out);
    TEST_ASSERT(out.f32[0] == -2.0f, "VROUNDSS negative ties to even");
    RUN_ROUND_SS("1", out);
    TEST_ASSERT(out.f32[0] == -3.0f, "VROUNDSS round down negative");
    RUN_ROUND_SS("2", out);
    TEST_ASSERT(out.f32[0] == -2.0f, "VROUNDSS round up negative");

    src2.f64[0] = 3.75;
    __asm__ volatile(
        "vmovapd %1, %%xmm0\n\t"
        "vroundsd $0, %%xmm0, %%xmm1, %%xmm2\n\t"
        "vmovapd %%xmm2, %0"
        : "=m"(out) : "m"(src2) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(out.f64[0] == 4.0, "VROUNDSD round to nearest even");
#undef RUN_ROUND_SS
}

static void test_scalar_range_reduce(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /* VRANGESS: the value is the register source, the bound the other one. */
    src1.f32[0] = -2.0f;
    src2.f32[0] = 4.0f;
    /* The result keeps the sign of the value unless imm8[3] selects the
     * sign of the bound instead. */
    RUN_SS_IMM("vrangess", "0", out);
    TEST_ASSERT(out.f32[0] == -2.0f, "VRANGESS minimum keeps the value sign");
    RUN_SS_IMM("vrangess", "1", out);
    TEST_ASSERT(out.f32[0] == -4.0f, "VRANGESS maximum keeps the value sign");
    RUN_SS_IMM("vrangess", "8", out);
    TEST_ASSERT(out.f32[0] == 2.0f, "VRANGESS minimum uses the bound sign");
    RUN_SS_IMM("vrangess", "9", out);
    TEST_ASSERT(out.f32[0] == 4.0f, "VRANGESS maximum uses the bound sign");
    /* imm8[2] compares the absolute values. */
    RUN_SS_IMM("vrangess", "4", out);
    TEST_ASSERT(out.f32[0] == -2.0f, "VRANGESS minimum of absolute values");

    /* VREDUCESS: value minus the rounded value. */
    src2.f32[0] = 2.5f;
    RUN_SS_IMM("vreducess", "0", out);
    TEST_ASSERT(out.f32[0] == src2.f32[0] - nearbyintf(src2.f32[0]),
                "VREDUCESS fractional part");
    RUN_SS_IMM("vreducess", "3", out);
    TEST_ASSERT(out.f32[0] == src2.f32[0] - truncf(src2.f32[0]),
                "VREDUCESS truncation");

    /* VRNDSCALESS rounding modes.  Only the rounding field of the immediate
     * is exercised because the scale field is not applied by the hardware. */
    RUN_SS_IMM("vrndscaless", "0", out);
    TEST_ASSERT(out.f32[0] == 2.0f, "VRNDSCALESS round to nearest even");
    RUN_SS_IMM("vrndscaless", "1", out);
    TEST_ASSERT(out.f32[0] == 2.0f, "VRNDSCALESS round down");
    RUN_SS_IMM("vrndscaless", "2", out);
    TEST_ASSERT(out.f32[0] == 3.0f, "VRNDSCALESS round up");
    RUN_SS_IMM("vrndscaless", "3", out);
    TEST_ASSERT(out.f32[0] == 2.0f, "VRNDSCALESS truncate");

    src2.f64[0] = 3.75;
    RUN_SD_IMM("vreducesd", "0", out);
    TEST_ASSERT(out.f64[0] == 3.75 - nearbyint(3.75), "VREDUCESD fractional part");
}

static void test_scalar_exp_mant_scale(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /* VGETEXPSS returns the exponent of the input. */
    src2.f32[0] = 8.0f;
    RUN_SS("vgetexpss", out);
    TEST_ASSERT(out.f32[0] == 3.0f, "VGETEXPSS exponent of 8");

    src2.f32[0] = 0.5f;
    RUN_SS("vgetexpss", out);
    TEST_ASSERT(out.f32[0] == -1.0f, "VGETEXPSS exponent of 0.5");

    /* VGETMANTSS returns the normalized mantissa. */
    src2.f32[0] = 100.0f;
    RUN_SS_IMM("vgetmantss", "0", out);
    TEST_ASSERT(out.f32[0] == 1.5625f, "VGETMANTSS mantissa of 100");
    src2.f32[0] = 0.75f;
    RUN_SS_IMM("vgetmantss", "0", out);
    TEST_ASSERT(out.f32[0] == 1.5f, "VGETMANTSS mantissa of 0.75");

    /* VSCALEFSS multiplies the register source by 2 raised to the other. */
    src1.f32[0] = 2.0f;
    src2.f32[0] = 3.0f;
    RUN_SS("vscalefss", out);
    TEST_ASSERT(out.f32[0] == 16.0f, "VSCALEFSS value 2 scaled by 2^3");

    src2.f32[0] = -2.0f;
    RUN_SS("vscalefss", out);
    TEST_ASSERT(out.f32[0] == 0.5f, "VSCALEFSS negative scale");

    src1.f64[0] = 1.5;
    src2.f64[0] = 3.0;
    RUN_SD("vscalefsd", out);
    TEST_ASSERT(out.f64[0] == 12.0, "VSCALEFSD value 1.5 scaled by 2^3");
}

#define RUN_FPCLASS_SS(imm, src, mask_out)                                      \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "vmovaps %1, %%xmm0\n\t"                                             \
            "vfpclassss $" imm ", %%xmm0, %%k1\n\t"                             \
            "kmovw %%k1, %0"                                                    \
            : "=r"(mask_out) : "m"(src) : "xmm0", "k1");                        \
    } while (0)

#define RUN_FPCLASS_SD(imm, src, mask_out)                                      \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "vmovapd %1, %%xmm0\n\t"                                             \
            "vfpclasssd $" imm ", %%xmm0, %%k1\n\t"                             \
            "kmovw %%k1, %0"                                                    \
            : "=r"(mask_out) : "m"(src) : "xmm0", "k1");                        \
    } while (0)

static void test_scalar_fpclass(void) {
    xmm_t src;
    uint32_t mask;

    /* The scalar form classifies the lowest element only. */
    src.u32[0] = UINT32_C(0x7fc12345); /* QNaN */
    RUN_FPCLASS_SS("0x01", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS QNaN bit");
    RUN_FPCLASS_SS("0xff", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS combined immediate");

    src.u32[0] = UINT32_C(0x00000000); /* +0 */
    RUN_FPCLASS_SS("0x02", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS +0 bit");
    RUN_FPCLASS_SS("0x08", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0000U, "VFPCLASSSS +0 is not +Inf");

    src.u32[0] = UINT32_C(0x80000000); /* -0 */
    RUN_FPCLASS_SS("0x04", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS -0 bit");

    src.u32[0] = UINT32_C(0x7f800000); /* +Inf */
    RUN_FPCLASS_SS("0x08", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS +Inf bit");
    RUN_FPCLASS_SS("0x10", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0000U, "VFPCLASSSS +Inf is not -Inf");

    src.u32[0] = UINT32_C(0xff800000); /* -Inf */
    RUN_FPCLASS_SS("0x10", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS -Inf bit");

    src.u32[0] = UINT32_C(0xffc00000); /* -QNaN */
    RUN_FPCLASS_SS("0x01", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS negative QNaN");

    src.u32[0] = UINT32_C(0x00000001); /* positive subnormal */
    RUN_FPCLASS_SS("0x20", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS subnormal bit");
    RUN_FPCLASS_SS("0x40", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0000U, "VFPCLASSSS positive subnormal");

    src.u32[0] = UINT32_C(0x80000001); /* negative subnormal */
    RUN_FPCLASS_SS("0x20", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS negative subnormal");
    RUN_FPCLASS_SS("0x40", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSS negative bit");

    /* Double precision classification uses the same immediate layout. */
    src.u64[0] = UINT64_C(0x7ff8000000000000); /* QNaN */
    RUN_FPCLASS_SD("0x01", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSD QNaN bit");
    src.u64[0] = UINT64_C(0x8000000000000000); /* -0 */
    RUN_FPCLASS_SD("0x04", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSD -0 bit");
    src.u64[0] = UINT64_C(0x7ff0000000000000); /* +Inf */
    RUN_FPCLASS_SD("0x08", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSD +Inf bit");
    RUN_FPCLASS_SD("0xff", src, mask);
    TEST_ASSERT((mask & 0xffffU) == 0x0001U, "VFPCLASSSD combined immediate");
}

static void test_scalar_conversions(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /* VCVTSD2SS converts the memory capable source to single precision. */
    src2.f64[0] = 1.5;
    src2.f64[1] = 0.0;
    RUN_SS("vcvtsd2ss", out);
    TEST_ASSERT(out.f32[0] == 1.5f, "VCVTSD2SS exact conversion");
    TEST_ASSERT(out.u32[1] == SRC1_UPPER_PATTERN,
                "VCVTSD2SS copies the upper bits");

    /* Rounding to nearest even when the value is not representable. */
    src2.f64[0] = 0x1.0000000000001p+0; /* just above 1.0 */
    src2.f64[1] = 0.0;
    RUN_SS("vcvtsd2ss", out);
    TEST_ASSERT(out.f32[0] == 1.0f, "VCVTSD2SS rounds to nearest even");

    /* VCVTSS2SD widens the memory capable source. */
    src2.f32[0] = 2.25f;
    src2.f32[1] = 0.0f;
    src2.f32[2] = 0.0f;
    src2.f32[3] = 0.0f;
    RUN_SS("vcvtss2sd", out);
    TEST_ASSERT(out.f64[0] == 2.25, "VCVTSS2SD widening conversion");
}

static void test_scalar_integer_conversions(void) {
    uint64_t unsigned_result = 0;
    uint32_t mask = 0xffffU;

    init_operands();

    /* VCVTTSD2USI converts to an unsigned 64 bit integer and saturates to
     * all ones when the value is out of range. */
    src2.f64[0] = 42.5;
    src2.f64[1] = 0.0;
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %1, %%xmm0\n\t"
        "vcvttsd2usi %%xmm0, %%rax\n\t"
        "mov %%rax, %0"
        : "=r"(unsigned_result)
        : "m"(src2), "m"(dst_init), "r"(mask)
        : "xmm0", "k1", "rax");
    TEST_ASSERT(unsigned_result == 42U, "VCVTTSD2USI truncates toward zero");

    src2.f64[0] = -1.0;
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %1, %%xmm0\n\t"
        "vcvttsd2usi %%xmm0, %%rax\n\t"
        "mov %%rax, %0"
        : "=r"(unsigned_result)
        : "m"(src2), "m"(dst_init), "r"(mask)
        : "xmm0", "k1", "rax");
    TEST_ASSERT(unsigned_result == UINT64_C(0xffffffffffffffff),
                "VCVTTSD2USI of a negative value saturates to all ones");

    src2.f64[0] = 1e19;
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %1, %%xmm0\n\t"
        "vcvttsd2usi %%xmm0, %%rax\n\t"
        "mov %%rax, %0"
        : "=r"(unsigned_result)
        : "m"(src2), "m"(dst_init), "r"(mask)
        : "xmm0", "k1", "rax");
    TEST_ASSERT(unsigned_result == UINT64_C(0x8ac7230489e80000),
                "VCVTTSD2USI of 1e19");

    /* VCVTUSI2SD converts an unsigned 64 bit integer to double precision. */
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %4, %%xmm3\n\t"
        "vcvtusi2sd %5, %%xmm3, %%xmm2\n\t"
        "vmovapd %%xmm2, %0"
        : "=m"(out)
        : "m"(src2), "m"(dst_init), "r"(mask), "m"(dst_init),
          "r"(UINT64_C(1234567890123))
        : "xmm2", "xmm3", "k1");
    TEST_ASSERT(out.f64[0] == 1234567890123.0, "VCVTUSI2SD unsigned to double");

    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %4, %%xmm3\n\t"
        "vcvtusi2sd %5, %%xmm3, %%xmm2\n\t"
        "vmovapd %%xmm2, %0"
        : "=m"(out)
        : "m"(src2), "m"(dst_init), "r"(mask), "m"(dst_init),
          "r"(UINT64_C(0xffffffffffffffff))
        : "xmm2", "xmm3", "k1");
    TEST_ASSERT(out.f64[0] == 18446744073709551616.0,
                "VCVTUSI2SD rounds the largest unsigned value");

    /* VCVTUSI2SS converts an unsigned 64 bit integer to single precision. */
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovaps %4, %%xmm3\n\t"
        "vcvtusi2ss %5, %%xmm3, %%xmm2\n\t"
        "vmovaps %%xmm2, %0"
        : "=m"(out)
        : "m"(src2), "m"(dst_init), "r"(mask), "m"(dst_init), "r"(UINT64_C(1000000))
        : "xmm2", "xmm3", "k1");
    TEST_ASSERT(out.f32[0] == 1000000.0f, "VCVTUSI2SS unsigned to single");
}

int main(void) {
    TEST_START("EVEX AVX-512 scalar floating point");
    if (!CHECK_AVX512F()) return 0;
    test_scalar_add_sub_mul_div();
    test_scalar_masking();
    test_scalar_sqrt_reciprocal();
    test_scalar_round();
    test_scalar_range_reduce();
    test_scalar_exp_mant_scale();
    test_scalar_fpclass();
    test_scalar_conversions();
    test_scalar_integer_conversions();
    TEST_END();
}
