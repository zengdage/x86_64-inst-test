/*
 * test_mulsd_divsd.c - Test SSE2 MULSD/DIVSD instructions
 *
 * MULSD: Multiply scalar double-precision float (lowest 64 bits of XMM).
 * DIVSD: Divide scalar double-precision float.
 * Upper 64 bits of destination are preserved.
 *
 * Compile: gcc -o test_mulsd_divsd float/test_mulsd_divsd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_mulsd(void) {
    xmm_t a, b, result;

    /* Basic multiply */
    a.f64[0] = 3.0; a.f64[1] = 100.0;
    b.f64[0] = 4.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 12.0, "mulsd 3*4=12: got %f", result.f64[0]);
    TEST_ASSERT(result.f64[1] == 100.0, "mulsd upper preserved");

    /* Multiply by zero */
    a.f64[0] = 42.0;
    b.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 0.0, "mulsd x*0=0");

    /* Inf * 0 = NaN */
    a.f64[0] = INFINITY;
    b.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f64[0]), "mulsd inf*0=NaN");

    /* Negative * Negative */
    a.f64[0] = -7.0;
    b.f64[0] = -3.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 21.0, "mulsd (-7)*(-3)=21");

    /* NaN */
    a.f64[0] = NAN;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f64[0]), "mulsd NaN*5=NaN");
}

static void test_mulsd_mem(void) {
    xmm_t a, result;
    double mem_val = 5.0;

    a.f64[0] = 6.0; a.f64[1] = 50.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "mulsd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 30.0, "mulsd xmm,mem 6*5=30");
    TEST_ASSERT(result.f64[1] == 50.0, "mulsd xmm,mem upper preserved");
}

static void test_divsd(void) {
    xmm_t a, b, result;

    /* Basic division */
    a.f64[0] = 10.0; a.f64[1] = 200.0;
    b.f64[0] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "divsd 10/2=5: got %f", result.f64[0]);
    TEST_ASSERT(result.f64[1] == 200.0, "divsd upper preserved");

    /* x / 0 = Inf */
    a.f64[0] = 1.0;
    b.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f64[0]) && result.f64[0] > 0, "divsd 1/0=+inf");

    /* 0/0 = NaN */
    a.f64[0] = 0.0;
    b.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f64[0]), "divsd 0/0=NaN");

    /* Inf / Inf = NaN */
    a.f64[0] = INFINITY;
    b.f64[0] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f64[0]), "divsd inf/inf=NaN");

    /* Denormal result */
    a.f64[0] = DBL_MIN;
    b.f64[0] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == DBL_MIN / 2.0, "divsd DBL_MIN/2=denormal");
}

static void test_divsd_mem(void) {
    xmm_t a, result;
    double mem_val = 4.0;

    a.f64[0] = 20.0; a.f64[1] = 50.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "divsd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "divsd xmm,mem 20/4=5");
    TEST_ASSERT(result.f64[1] == 50.0, "divsd xmm,mem upper preserved");
}

int main(void) {
    TEST_START("MULSD/DIVSD instructions");
    test_mulsd();
    test_mulsd_mem();
    test_divsd();
    test_divsd_mem();
    TEST_END();
}
