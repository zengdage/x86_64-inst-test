/*
 * test_mulpd_divpd.c - Test SSE2 MULPD/DIVPD instructions
 *
 * MULPD: Multiply packed double-precision floats (2 x 64-bit parallel).
 * DIVPD: Divide packed double-precision floats.
 *
 * Compile: gcc -o test_mulpd_divpd float/test_mulpd_divpd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_mulpd_basic(void) {
    xmm_t a, b, result;

    a.f64[0] = 3.0; a.f64[1] = 5.0;
    b.f64[0] = 4.0; b.f64[1] = 6.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 12.0, "mulpd [0] 3*4=12");
    TEST_ASSERT(result.f64[1] == 30.0, "mulpd [1] 5*6=30");
}

static void test_mulpd_special(void) {
    xmm_t a, b, result;

    a.f64[0] = INFINITY; a.f64[1] = NAN;
    b.f64[0] = 0.0;      b.f64[1] = 42.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f64[0]), "mulpd inf*0=NaN");
    TEST_ASSERT(isnan(result.f64[1]), "mulpd NaN*42=NaN");

    /* Negative * Negative */
    a.f64[0] = -7.0; a.f64[1] = -3.0;
    b.f64[0] = -3.0; b.f64[1] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "mulpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 21.0, "mulpd (-7)*(-3)=21");
    TEST_ASSERT(result.f64[1] == -15.0, "mulpd (-3)*5=-15");
}

static void test_mulpd_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f64[0] = 5.0; a.f64[1] = 10.0;
    mem.f64[0] = 2.0; mem.f64[1] = 3.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "mulpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 10.0, "mulpd xmm,mem [0] 5*2=10");
    TEST_ASSERT(result.f64[1] == 30.0, "mulpd xmm,mem [1] 10*3=30");
}

static void test_divpd_basic(void) {
    xmm_t a, b, result;

    a.f64[0] = 10.0; a.f64[1] = 20.0;
    b.f64[0] = 2.0;  b.f64[1] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "divpd [0] 10/2=5");
    TEST_ASSERT(result.f64[1] == 4.0, "divpd [1] 20/5=4");
}

static void test_divpd_special(void) {
    xmm_t a, b, result;

    a.f64[0] = 1.0; a.f64[1] = 0.0;
    b.f64[0] = 0.0; b.f64[1] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f64[0]) && result.f64[0] > 0, "divpd 1/0=+inf");
    TEST_ASSERT(isnan(result.f64[1]), "divpd 0/0=NaN");

    /* Inf / Inf; denormal / 2 */
    a.f64[0] = INFINITY; a.f64[1] = DBL_MIN;
    b.f64[0] = INFINITY; b.f64[1] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "divpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f64[0]), "divpd inf/inf=NaN");
    TEST_ASSERT(result.f64[1] == DBL_MIN / 2.0, "divpd DBL_MIN/2=denormal");
}

static void test_divpd_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f64[0] = 100.0; a.f64[1] = 200.0;
    mem.f64[0] = 10.0; mem.f64[1] = 20.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "divpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 10.0, "divpd xmm,mem [0] 100/10=10");
    TEST_ASSERT(result.f64[1] == 10.0, "divpd xmm,mem [1] 200/20=10");
}

int main(void) {
    TEST_START("MULPD/DIVPD instructions");
    test_mulpd_basic();
    test_mulpd_special();
    test_mulpd_mem();
    test_divpd_basic();
    test_divpd_special();
    test_divpd_mem();
    TEST_END();
}
