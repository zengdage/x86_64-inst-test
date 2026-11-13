/*
 * EVEX-encoded AVX-512 packed double precision math instructions.
 *
 * Covers VGETEXPPD, VGETMANTPD, VRNDSCALEPD, VSCALEFPD, VRANGEPD,
 * VREDUCEPD, VFPCLASSPD, VRCP14PD and VRSQRT14PD, including the merge and
 * zero masking behaviour of the packed destinations.
 */
#include "avx512_test.h"
#include <float.h>
#include <math.h>

static zmm_test_t src1, src2, dst_init, out;

static void init_operands(void) {
    for (int i = 0; i < 8; i++) {
        src1.u64[i] = UINT64_C(0xaaaaaaaaaaaaaaaa);
        src2.u64[i] = UINT64_C(0x5555555555555555);
        dst_init.u64[i] = UINT64_C(0xcccccccccccccccc);
    }
}

/* Two operand packed form: source and destination. */
#define RUN_PD(ins, out)                                                        \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %2, %%k1\n\t"                                                \
            "vmovapd %1, %%zmm0\n\t"                                             \
            "vmovapd %3, %%zmm2\n\t"                                             \
            ins " %%zmm0, %%zmm2%{%%k1%}\n\t"                                    \
            "vmovapd %%zmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "r"(mask), "m"(dst_init)                               \
            : "zmm0", "zmm2", "k1");                                             \
    } while (0)

/* Two operand packed form with zero masking. */
#define RUN_PD_ZERO(ins, out)                                                   \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %2, %%k1\n\t"                                                \
            "vmovapd %1, %%zmm0\n\t"                                             \
            "vmovapd %3, %%zmm2\n\t"                                             \
            ins " %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"                               \
            "vmovapd %%zmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "r"(mask), "m"(dst_init)                               \
            : "zmm0", "zmm2", "k1");                                             \
    } while (0)

/* Three operand packed form: two sources and the destination. */
#define RUN_PD3(ins, out)                                                       \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovapd %1, %%zmm0\n\t"                                             \
            "vmovapd %2, %%zmm1\n\t"                                             \
            "vmovapd %4, %%zmm2\n\t"                                             \
            ins " %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"                           \
            "vmovapd %%zmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "zmm0", "zmm1", "zmm2", "k1");                                     \
    } while (0)

/* Packed form with an immediate and a single source. */
#define RUN_PD_IMM(ins, imm, out)                                               \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %2, %%k1\n\t"                                                \
            "vmovapd %1, %%zmm0\n\t"                                             \
            "vmovapd %3, %%zmm2\n\t"                                             \
            ins " $" imm ", %%zmm0, %%zmm2%{%%k1%}\n\t"                          \
            "vmovapd %%zmm2, %0"                                                \
            : "=m"(out)                                                         \
            : "m"(src1), "r"(mask), "m"(dst_init)                               \
            : "zmm0", "zmm2", "k1");                                             \
    } while (0)

static void test_getexp_getmant(void) {
    static const double values[8] = {
        8.0, 0.5, 1.0, 100.0, 0.75, 1e300, 2.0, 0.125
    };
    static const double expected_exp[8] = {3, -1, 0, 6, -1, 996, 1, -3};
    uint32_t mask = 0xffffU;

    init_operands();
    for (int i = 0; i < 8; i++) src1.f64[i] = values[i];

    RUN_PD("vgetexppd", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == expected_exp[i], "VGETEXPPD lane %d", i);

    RUN_PD_IMM("vgetmantpd", "0", out);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(out.f64[i] >= 1.0 && out.f64[i] < 2.0,
                    "VGETMANTPD lane %d is normalized: %a", i, out.f64[i]);
        TEST_ASSERT(ldexp(out.f64[i], (int)expected_exp[i]) == values[i],
                    "VGETMANTPD lane %d reconstructs the input", i);
    }

    /* Zero masking clears the masked out lanes. */
    mask = 0x0fU;
    RUN_PD_ZERO("vgetexppd", out);
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.f64[i] == expected_exp[i], "VGETEXPPD zero mask lane %d", i);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(out.f64[i] == 0.0, "VGETEXPPD zero mask clears lane %d", i);
}

static void test_scalef_range(void) {
    static const double values[8] = {2.0, 3.0, -1.5, 0.25, 10.0, 0.5, 4.0, -8.0};
    static const double scales[8] = {3.0, -2.0, 1.0, 0.0, 4.0, -1.0, 2.0, 5.0};
    uint32_t mask = 0xffffU;

    init_operands();
    for (int i = 0; i < 8; i++) {
        src1.f64[i] = values[i];
        src2.f64[i] = scales[i];
    }

    RUN_PD3("vscalefpd", out);
    for (int i = 0; i < 8; i++) {
        double expected = ldexp(values[i], (int)scales[i]);
        TEST_ASSERT(out.f64[i] == expected, "VSCALEFPD lane %d: %a != %a",
                    i, out.f64[i], expected);
    }

    /* VRANGEPD compares the value with the bound and keeps the sign of the
     * value unless imm8[3] selects the sign of the bound. */
    for (int i = 0; i < 8; i++) {
        src1.f64[i] = -2.0;
        src2.f64[i] = 4.0;
    }
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vrangepd $0, %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(out) : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == -2.0, "VRANGEPD minimum lane %d", i);

    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vrangepd $1, %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(out) : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == -4.0, "VRANGEPD maximum lane %d", i);

    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vrangepd $9, %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(out) : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == 4.0, "VRANGEPD bound sign lane %d", i);
}

static void test_reduce_round(void) {
    static const double values[8] = {
        2.5, -2.5, 3.75, -3.75, 0.5, -0.5, 10.25, -10.25
    };
    uint32_t mask = 0xffffU;

    init_operands();
    for (int i = 0; i < 8; i++) src1.f64[i] = values[i];

    RUN_PD_IMM("vreducepd", "0", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == values[i] - nearbyint(values[i]),
                    "VREDUCEPD lane %d", i);

    RUN_PD_IMM("vreducepd", "3", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == values[i] - trunc(values[i]),
                    "VREDUCEPD truncating lane %d", i);

    /* VRNDSCALEPD rounding modes. */
    RUN_PD_IMM("vrndscalepd", "0", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == nearbyint(values[i]),
                    "VRNDSCALEPD round to nearest even lane %d", i);

    RUN_PD_IMM("vrndscalepd", "1", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == floor(values[i]),
                    "VRNDSCALEPD round down lane %d", i);

    RUN_PD_IMM("vrndscalepd", "2", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == ceil(values[i]),
                    "VRNDSCALEPD round up lane %d", i);

    RUN_PD_IMM("vrndscalepd", "3", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == trunc(values[i]),
                    "VRNDSCALEPD truncate lane %d", i);
}

static void test_reciprocal(void) {
    static const double values[8] = {1.0, 2.0, 4.0, 9.0, 16.0, 0.25, 100.0, 0.0625};
    uint32_t mask = 0xffffU;

    init_operands();
    for (int i = 0; i < 8; i++) src1.f64[i] = values[i];

    RUN_PD("vrcp14pd", out);
    for (int i = 0; i < 8; i++) {
        double exact = 1.0 / values[i];
        TEST_ASSERT(fabs(out.f64[i] - exact) <= fabs(exact) * 0x1p-14,
                    "VRCP14PD lane %d: %a != %a", i, out.f64[i], exact);
    }

    RUN_PD("vrsqrt14pd", out);
    for (int i = 0; i < 8; i++) {
        double exact = 1.0 / sqrt(values[i]);
        TEST_ASSERT(fabs(out.f64[i] - exact) <= fabs(exact) * 0x1p-14,
                    "VRSQRT14PD lane %d: %a != %a", i, out.f64[i], exact);
    }
}

#define RUN_FPCLASS_PD(imm, src, mask_out)                                      \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "vmovapd %1, %%zmm0\n\t"                                             \
            "vfpclasspd $" imm ", %%zmm0, %%k1\n\t"                             \
            "kmovq %%k1, %0"                                                    \
            : "=r"(mask_out) : "m"(src) : "zmm0", "k1");                        \
    } while (0)

static void test_fpclass(void) {
    zmm_test_t src;
    uint64_t mask;

    src.u64[0] = UINT64_C(0x7ff8000000000000); /* QNaN */
    src.u64[1] = UINT64_C(0x0000000000000000); /* +0 */
    src.u64[2] = UINT64_C(0x8000000000000000); /* -0 */
    src.u64[3] = UINT64_C(0x7ff0000000000000); /* +Inf */
    src.u64[4] = UINT64_C(0xfff0000000000000); /* -Inf */
    src.u64[5] = UINT64_C(0x0000000000000001); /* positive subnormal */
    src.u64[6] = UINT64_C(0x8000000000000001); /* negative subnormal */
    src.u64[7] = UINT64_C(0x7ff0000000000001); /* SNaN */

    RUN_FPCLASS_PD("0x01", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x01U, "VFPCLASSPD QNaN bits");
    RUN_FPCLASS_PD("0x02", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x02U, "VFPCLASSPD +0 bit");
    RUN_FPCLASS_PD("0x04", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x04U, "VFPCLASSPD -0 bit");
    RUN_FPCLASS_PD("0x08", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x08U, "VFPCLASSPD +Inf bit");
    RUN_FPCLASS_PD("0x10", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x10U, "VFPCLASSPD -Inf bit");
    RUN_FPCLASS_PD("0x20", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x60U, "VFPCLASSPD subnormal bits");
    /* The negative class covers negative numbers but neither -0 nor -Inf;
     * a negative denormal belongs to both the denormal and negative class. */
    RUN_FPCLASS_PD("0x40", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x40U, "VFPCLASSPD negative bits");
    RUN_FPCLASS_PD("0x80", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0x80U, "VFPCLASSPD SNaN bit");
    RUN_FPCLASS_PD("0xff", src, mask);
    TEST_ASSERT((mask & 0xffU) == 0xffU, "VFPCLASSPD combined immediate");
}

int main(void) {
    TEST_START("EVEX AVX-512 packed double precision math");
    if (!CHECK_AVX512F()) return 0;
    test_getexp_getmant();
    test_scalef_range();
    test_reduce_round();
    test_reciprocal();
    test_fpclass();
    TEST_END();
}
