/* AVX-512 double-precision FMA3 forms not covered elsewhere:
 * VFMADD213PD, VFMSUB132/213/231PD, VFNMADD132/213/231PD and
 * VFNMSUB132/213/231PD, including fused rounding and mask behaviour.
 */
#include "../include/avx512_test.h"
#include <float.h>
#include <math.h>

/* a is the destination, b the first source and c the second source. */
#define RUN_FMA(inst, expression) do { \
    __asm__ volatile( \
        "vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t" \
        "vmovapd %3, %%zmm2\n\t" #inst " %%zmm2, %%zmm1, %%zmm0\n\t" \
        "vmovapd %%zmm0, %0" \
        : "=m"(out) : "m"(a), "m"(b), "m"(c) \
        : "zmm0", "zmm1", "zmm2"); \
    for (int lane = 0; lane < 8; lane++) { \
        double expected = (expression); \
        TEST_ASSERT(out.f64[lane] == expected, #inst " lane %d: %a != %a", \
                    lane, out.f64[lane], expected); \
    } \
} while (0)

static void test_encoding_orders(void) {
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)(i + 1);
        b.f64[i] = (double)(2 * i - 7);
        c.f64[i] = (double)(i + 3);
    }
    RUN_FMA(vfmadd213pd, b.f64[lane] * a.f64[lane] + c.f64[lane]);
    RUN_FMA(vfmsub132pd, a.f64[lane] * c.f64[lane] - b.f64[lane]);
    RUN_FMA(vfmsub213pd, b.f64[lane] * a.f64[lane] - c.f64[lane]);
    RUN_FMA(vfmsub231pd, b.f64[lane] * c.f64[lane] - a.f64[lane]);
    RUN_FMA(vfnmadd132pd, -(a.f64[lane] * c.f64[lane]) + b.f64[lane]);
    RUN_FMA(vfnmadd213pd, -(b.f64[lane] * a.f64[lane]) + c.f64[lane]);
    RUN_FMA(vfnmadd231pd, -(b.f64[lane] * c.f64[lane]) + a.f64[lane]);
    RUN_FMA(vfnmsub132pd, -(a.f64[lane] * c.f64[lane]) - b.f64[lane]);
    RUN_FMA(vfnmsub213pd, -(b.f64[lane] * a.f64[lane]) - c.f64[lane]);
    RUN_FMA(vfnmsub231pd, -(b.f64[lane] * c.f64[lane]) - a.f64[lane]);
#undef RUN_FMA
}

static void test_fused_rounding(void) {
    zmm_test_t a, b, c, out;
    /* (1 + 2^-52) * (2 - 2^-52) is inexact, so the fused result differs from
     * a separately rounded multiply followed by a rounded add. */
    a.f64[0] = 0x1.fffffffffffffp+0;
    b.f64[0] = 0x1.0000000000001p+0;
    c.f64[0] = -1.0;
    for (int i = 1; i < 8; i++) {
        a.f64[i] = (double)i;
        b.f64[i] = 1.0;
        c.f64[i] = 2.0;
    }

    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vmovapd %3, %%zmm2\n\t"
                     "vfmadd213pd %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovapd %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(out.f64[0] == 0x1.0000000000001p+0,
                "VFMADD213PD keeps the fused residual: %a", out.f64[0]);
    volatile double product = b.f64[0] * a.f64[0];
    volatile double separately = product + c.f64[0];
    TEST_ASSERT(separately == 1.0,
                "separately rounded multiply gives %a", separately);
    TEST_ASSERT(separately != out.f64[0],
                "separately rounded multiply differs from the fused result");

    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vmovapd %3, %%zmm2\n\t"
                     "vfnmsub213pd %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovapd %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(out.f64[0] == -0x1.0000000000001p+0,
                "VFNMSUB213PD keeps the fused residual: %a", out.f64[0]);
    volatile double product2 = b.f64[0] * a.f64[0];
    volatile double separately2 = -product2 - c.f64[0];
    TEST_ASSERT(separately2 == -1.0,
                "separately rounded negate-multiply gives %a", separately2);
    TEST_ASSERT(separately2 != out.f64[0],
                "separately rounded negate-multiply differs from the fused result");
}

static void test_special_values(void) {
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = 1.0;
        b.f64[i] = 2.0;
        c.f64[i] = 4.0;
    }
    a.f64[0] = INFINITY;
    c.f64[0] = 0.0;
    a.f64[1] = NAN;
    a.f64[2] = INFINITY;
    c.f64[2] = INFINITY;
    a.f64[3] = -0.0;
    b.f64[3] = 0.0;

    /* VFMADD213PD computes src1 * dest + src2. */
    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vmovapd %3, %%zmm2\n\t"
                     "vfmadd213pd %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovapd %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(isinf(out.f64[0]) && !signbit(out.f64[0]),
                "VFMADD213PD infinity times a finite value stays infinite");
    TEST_ASSERT(IS_QNAN(out.f64[1]), "VFMADD213PD propagates QNaN");
    TEST_ASSERT(isinf(out.f64[2]) && !signbit(out.f64[2]),
                "VFMADD213PD infinity times infinity stays infinite");
    TEST_ASSERT(out.f64[3] == 4.0, "VFMADD213PD -0 + 0 keeps the addend: %a", out.f64[3]);

    /* VFNMSUB231PD computes -(src1 * src2) - dest. */
    a.f64[4] = 0.0;
    b.f64[4] = 0.0;
    c.f64[4] = 4.0;
    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vmovapd %3, %%zmm2\n\t"
                     "vfnmsub231pd %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovapd %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(out.f64[4] == -0.0 && signbit(out.f64[4]),
                "VFNMSUB231PD of zero produces negative zero");
    TEST_ASSERT(out.f64[5] == -(2.0 * 4.0) - 1.0, "VFNMSUB231PD lane 5");
    TEST_ASSERT(IS_QNAN(out.f64[1]), "VFNMSUB231PD propagates QNaN");
}

static void test_masking(void) {
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)(i + 1);
        b.f64[i] = 2.0;
        c.f64[i] = 3.0;
    }
    uint32_t mask = UINT32_C(0x81);

    /* Merge masking keeps the previous destination content, which here is
     * the first source operand of the FMA. */
    __asm__ volatile("kmovd %1, %%k1\n\t"
                     "vmovapd %2, %%zmm0\n\tvmovapd %3, %%zmm1\n\t"
                     "vmovapd %4, %%zmm2\n\t"
                     "vfmsub132pd %%zmm2, %%zmm1, %%zmm0%{%%k1%}\n\t"
                     "vmovapd %%zmm0, %0"
                     : "=m"(out) : "r"(mask), "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) {
        double expected = (i == 0 || i == 7)
                          ? a.f64[i] * c.f64[i] - b.f64[i] : a.f64[i];
        TEST_ASSERT(out.f64[i] == expected, "VFMSUB132PD merge mask lane %d", i);
    }

    __asm__ volatile("kmovd %1, %%k1\n\t"
                     "vmovapd %2, %%zmm0\n\tvmovapd %3, %%zmm1\n\t"
                     "vmovapd %4, %%zmm2\n\t"
                     "vfnmadd231pd %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
                     "vmovapd %%zmm0, %0"
                     : "=m"(out) : "r"(mask), "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) {
        double expected = (i == 0 || i == 7)
                          ? -(b.f64[i] * c.f64[i]) + a.f64[i] : 0.0;
        TEST_ASSERT(out.f64[i] == expected, "VFNMADD231PD zero mask lane %d", i);
    }
}

int main(void) {
    TEST_START("AVX-512 double-precision FMA forms");
    if (!CHECK_AVX512F()) return 0;
    test_encoding_orders();
    test_fused_rounding();
    test_special_values();
    test_masking();
    TEST_END();
}
