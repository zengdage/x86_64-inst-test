/*
 * EVEX-encoded AVX-512 scalar FMA instructions.
 *
 * Covers the single and double precision scalar forms of VFMADD132/213/231,
 * VFMSUB132/213/231, VFNMADD132/213/231 and VFNMSUB132/213/231.  The EVEX
 * encoding is selected by the destination mask, which also exercises the
 * merge and zero masking behaviour of the scalar destination.
 *
 * The operand order is the same as for the packed forms: the first operand
 * is the memory capable source, the second operand is the register source
 * and the third operand is the destination.  The 132 form multiplies the
 * destination with the first operand, the 213 form multiplies the destination
 * with the second operand and the 231 form multiplies the two sources.
 */
#include "../include/avx512_test.h"
#include <float.h>
#include <math.h>

static xmm_t src1, src2, dst_init, out;

static void init_operands(void) {
    for (int i = 0; i < 4; i++) {
        src1.u32[i] = UINT32_C(0xaaaaaaaa);
        src2.u32[i] = UINT32_C(0x55555555);
        dst_init.u32[i] = UINT32_C(0xcccccccc);
    }
}

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

/*
 * The 132, 213 and 231 forms share the same operand roles.  The first AT&T
 * operand is the memory capable source (src2 below), the second operand is
 * the register source (src1 below) and the third operand is the destination:
 *   132: destination = destination * src2 + src1
 *   213: destination = src1 * destination + src2
 *   231: destination = src1 * src2 + destination
 * The FMA forms leave the upper bits of the destination unmodified.
 */
#define CHECK_FMA(ins, expected)                                                \
    do {                                                                        \
        out.f32[0] = 0.0f;                                                     \
        RUN_SS(ins, out);                                                      \
        TEST_ASSERT(out.f32[0] == (float)(expected),                            \
                    ins " result %a != %a", out.f32[0], (float)(expected));     \
        TEST_ASSERT(out.u32[1] == UINT32_C(0xcccccccc),                         \
                    ins " keeps the upper bits of the destination");            \
    } while (0)

static void test_scalar_fma_single(void) {
    uint32_t mask = 0xffffU;

    init_operands();
    src1.f32[0] = 3.0f;
    src2.f32[0] = 5.0f;
    dst_init.f32[0] = 7.0f;

    CHECK_FMA("vfmadd132ss", 7.0f * 5.0f + 3.0f);
    CHECK_FMA("vfmadd213ss", 3.0f * 7.0f + 5.0f);
    CHECK_FMA("vfmadd231ss", 3.0f * 5.0f + 7.0f);
    CHECK_FMA("vfmsub132ss", 7.0f * 5.0f - 3.0f);
    CHECK_FMA("vfmsub213ss", 3.0f * 7.0f - 5.0f);
    CHECK_FMA("vfmsub231ss", 3.0f * 5.0f - 7.0f);
    CHECK_FMA("vfnmadd132ss", -(7.0f * 5.0f) + 3.0f);
    CHECK_FMA("vfnmadd213ss", -(3.0f * 7.0f) + 5.0f);
    CHECK_FMA("vfnmadd231ss", -(3.0f * 5.0f) + 7.0f);
    CHECK_FMA("vfnmsub132ss", -(7.0f * 5.0f) - 3.0f);
    CHECK_FMA("vfnmsub213ss", -(3.0f * 7.0f) - 5.0f);
    CHECK_FMA("vfnmsub231ss", -(3.0f * 5.0f) - 7.0f);

    /* Zero and infinity handling. */
    src1.f32[0] = 5.0f;
    src2.f32[0] = 0.0f;
    dst_init.f32[0] = INFINITY;
    RUN_SS("vfmadd132ss", out);
    TEST_ASSERT(IS_QNAN(out.f32[0]), "VFMADD132SS infinity times zero is QNaN");

    src1.f32[0] = 2.0f;
    src2.f32[0] = 1.0f;
    dst_init.f32[0] = INFINITY;
    RUN_SS("vfmadd132ss", out);
    TEST_ASSERT(isinf(out.f32[0]) && out.f32[0] > 0.0f,
                "VFMADD132SS keeps +Inf");

    /* A masked out destination is left untouched, zero masking clears it. */
    mask = 0U;
    dst_init.f32[0] = 11.0f;
    RUN_SS("vfmadd132ss", out);
    TEST_ASSERT(out.f32[0] == 11.0f,
                "VFMADD132SS merge mask zero keeps the destination");
    RUN_SS_ZERO("vfmadd132ss", out);
    TEST_ASSERT(out.f32[0] == 0.0f,
                "VFMADD132SS zero mask clears the destination element");
    TEST_ASSERT(out.u32[1] == UINT32_C(0xcccccccc),
                "VFMADD132SS zero mask keeps the upper bits");
}

static void test_scalar_fma_double(void) {
    uint32_t mask = 0xffffU;

    init_operands();
    src1.f64[0] = 3.0;
    src2.f64[0] = 5.0;
    dst_init.f64[0] = 7.0;

#define CHECK_FMA_D(ins, expected)                                              \
    do {                                                                        \
        out.f64[0] = 0.0;                                                      \
        RUN_SD(ins, out);                                                      \
        TEST_ASSERT(out.f64[0] == (double)(expected),                           \
                    ins " result %a != %a", out.f64[0], (double)(expected));    \
    } while (0)

    CHECK_FMA_D("vfmadd132sd", 7.0 * 5.0 + 3.0);
    CHECK_FMA_D("vfmadd213sd", 3.0 * 7.0 + 5.0);
    CHECK_FMA_D("vfmadd231sd", 3.0 * 5.0 + 7.0);
    CHECK_FMA_D("vfmsub132sd", 7.0 * 5.0 - 3.0);
    CHECK_FMA_D("vfmsub213sd", 3.0 * 7.0 - 5.0);
    CHECK_FMA_D("vfmsub231sd", 3.0 * 5.0 - 7.0);
    CHECK_FMA_D("vfnmadd132sd", -(7.0 * 5.0) + 3.0);
    CHECK_FMA_D("vfnmadd213sd", -(3.0 * 7.0) + 5.0);
    CHECK_FMA_D("vfnmadd231sd", -(3.0 * 5.0) + 7.0);
    CHECK_FMA_D("vfnmsub132sd", -(7.0 * 5.0) - 3.0);
    CHECK_FMA_D("vfnmsub213sd", -(3.0 * 7.0) - 5.0);
    CHECK_FMA_D("vfnmsub231sd", -(3.0 * 5.0) - 7.0);
#undef CHECK_FMA_D

    /* Fused rounding keeps the residual of a nearly cancelling product. */
    src1.f64[0] = -1.0;
    src2.f64[0] = 0x1.fffffffffffffp-1;
    dst_init.f64[0] = 0x1.0000000000001p+0;
    RUN_SD("vfmadd132sd", out);
    TEST_ASSERT(out.f64[0] == 0x1.ffffffffffffep-54,
                "VFMADD132SD keeps the fused residual: %a", out.f64[0]);
}

static void test_scalar_fma_fused_rounding(void) {
    uint32_t mask = 0xffffU;

    init_operands();
    /*
     * 1 + 2^-53 is not representable in single precision, so a separately
     * rounded multiply and add would lose the increment while the fused
     * form keeps it.
     */
    src1.f32[0] = 0x1.000002p+0f; /* 1 + 2^-23 */
    src2.f32[0] = 0x1.fffffep-1f;  /* 1 - 2^-24 */
    dst_init.f32[0] = 0x1.fffffep+0f;

    RUN_SS("vfmadd231ss", out);
    TEST_ASSERT(out.f32[0] == 3.0f, "VFMADD231SS single rounding: %a", out.f32[0]);

    /* The fused result of 1*1 + 2^-24 keeps the small addend. */
    src1.f32[0] = 0x1p-24f;
    src2.f32[0] = 1.0f;
    dst_init.f32[0] = 1.0f;
    RUN_SS("vfmadd231ss", out);
    TEST_ASSERT(out.f32[0] == 1.0f + 0x1p-24f,
                "VFMADD231SS keeps the fused addend: %a", out.f32[0]);
}

int main(void) {
    TEST_START("EVEX AVX-512 scalar FMA");
    if (!CHECK_AVX512F()) return 0;
    test_scalar_fma_single();
    test_scalar_fma_double();
    test_scalar_fma_fused_rounding();
    TEST_END();
}
