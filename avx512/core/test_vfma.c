/* EVEX FMA3 packed forms, fused rounding, masks, and operand aliasing. */
#include "../include/avx512_test.h"
#include <float.h>
#include <math.h>

static void test_encoding_orders(void) {
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 16; i++) {
        a.f32[i] = (float)(i + 1);
        b.f32[i] = (float)(2 * i - 7);
        c.f32[i] = (float)(i + 3);
    }
#define RUN_FMA(inst, expression) do { \
    __asm__ volatile( \
        "vmovaps %1, %%zmm0\n\tvmovaps %2, %%zmm1\n\t" \
        "vmovaps %3, %%zmm2\n\t" #inst " %%zmm2, %%zmm1, %%zmm0\n\t" \
        "vmovaps %%zmm0, %0" \
        : "=m"(out) : "m"(a), "m"(b), "m"(c) \
        : "zmm0", "zmm1", "zmm2"); \
    for (int lane = 0; lane < 16; lane++) { \
        float expected = (expression); \
        TEST_ASSERT(out.f32[lane] == expected, #inst " lane %d: %a != %a", \
                    lane, out.f32[lane], expected); \
    } \
} while (0)
    RUN_FMA(vfmadd132ps, a.f32[lane] * c.f32[lane] + b.f32[lane]);
    RUN_FMA(vfmadd213ps, b.f32[lane] * a.f32[lane] + c.f32[lane]);
    RUN_FMA(vfmadd231ps, b.f32[lane] * c.f32[lane] + a.f32[lane]);
    RUN_FMA(vfmsub132ps, a.f32[lane] * c.f32[lane] - b.f32[lane]);
    RUN_FMA(vfmsub213ps, b.f32[lane] * a.f32[lane] - c.f32[lane]);
    RUN_FMA(vfmsub231ps, b.f32[lane] * c.f32[lane] - a.f32[lane]);
    RUN_FMA(vfnmadd132ps, -(a.f32[lane] * c.f32[lane]) + b.f32[lane]);
    RUN_FMA(vfnmsub231ps, -(b.f32[lane] * c.f32[lane]) - a.f32[lane]);
#undef RUN_FMA
}

static void test_fused_special_and_mask(void) {
    zmm_test_t a, addend, multiplier, out;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)(i + 1);
        addend.f64[i] = 10.0;
        multiplier.f64[i] = 2.0;
    }
    a.f64[0] = 0x1.0000000000001p+0;
    addend.f64[0] = -1.0;
    multiplier.f64[0] = 0x1.fffffffffffffp-1;
    a.f64[1] = INFINITY;
    multiplier.f64[1] = 0.0;
    a.f64[2] = NAN;
    __asm__ volatile(
        "vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
        "vmovapd %3, %%zmm2\n\t"
        "vfmadd132pd %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovapd %%zmm0, %0"
        : "=m"(out) : "m"(a), "m"(addend), "m"(multiplier)
        : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(out.f64[0] == 0x1.ffffffffffffep-54,
                "VFMADD132PD preserves fused residual: %a", out.f64[0]);
    volatile double rounded = a.f64[0] * multiplier.f64[0];
    TEST_ASSERT(rounded + addend.f64[0] == 0.0,
                "separately rounded multiply/add cancels");
    TEST_ASSERT(IS_QNAN(out.f64[1]), "VFMADD132PD infinity times zero is QNaN");
    TEST_ASSERT(IS_QNAN(out.f64[2]), "VFMADD132PD propagates QNaN");

    uint32_t mask = 0x81U;
    for (int i = 0; i < 8; i++) out.f64[i] = -99.0;
    __asm__ volatile(
        "kmovb %4, %%k1\n\t"
        "vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
        "vmovapd %3, %%zmm2\n\t"
        "vfmadd231pd %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
        "vmovapd %%zmm0, %0"
        : "=m"(out) : "m"(a), "m"(addend), "m"(multiplier), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) {
        double expected = (i == 0 || i == 7)
                          ? addend.f64[i] * multiplier.f64[i] + a.f64[i] : 0.0;
        TEST_ASSERT(out.f64[i] == expected, "VFMADD231PD zero mask lane %d", i);
    }

    /* All operands aliased: x*x+x. */
    for (int i = 0; i < 16; i++) a.f32[i] = (float)(i - 8);
    __asm__ volatile(
        "vmovaps %1, %%zmm0\n\t"
        "vfmadd231ps %%zmm0, %%zmm0, %%zmm0\n\t"
        "vmovaps %%zmm0, %0"
        : "=m"(out) : "m"(a) : "zmm0");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.f32[i] == a.f32[i] * a.f32[i] + a.f32[i],
                    "VFMADD231PS all operands aliased lane %d", i);
}

int main(void) {
    TEST_START("EVEX VFMADD/VFMSUB/VFNMADD/VFNMSUB");
    if (!CHECK_AVX512F()) return 0;
    test_encoding_orders();
    test_fused_special_and_mask();
    TEST_END();
}
