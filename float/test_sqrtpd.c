/*
 * test_sqrtpd.c - Test SSE2 SQRTPD instruction
 *
 * SQRTPD: Packed double-precision square root (2 x 64-bit parallel).
 *
 * Compile: gcc -o test_sqrtpd float/test_sqrtpd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_sqrtpd_basic(void) {
    xmm_t a, result;

    a.f64[0] = 4.0; a.f64[1] = 9.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "sqrtpd [0] sqrt(4)=2");
    TEST_ASSERT(result.f64[1] == 3.0, "sqrtpd [1] sqrt(9)=3");
}

static void test_sqrtpd_special(void) {
    xmm_t a, result;

    /* Zero and Inf */
    a.f64[0] = 0.0; a.f64[1] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 0.0, "sqrtpd sqrt(0)=0");
    TEST_ASSERT(isinf(result.f64[1]) && result.f64[1] > 0, "sqrtpd sqrt(inf)=inf");

    /* Negative and NaN */
    a.f64[0] = -1.0; a.f64[1] = NAN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isnan(result.f64[0]), "sqrtpd sqrt(-1)=NaN");
    TEST_ASSERT(isnan(result.f64[1]), "sqrtpd sqrt(NaN)=NaN");

    /* -0 and 1 */
    a.f64[0] = -0.0; a.f64[1] = 1.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 0.0 && signbit(result.f64[0]), "sqrtpd sqrt(-0)=-0");
    TEST_ASSERT(result.f64[1] == 1.0, "sqrtpd sqrt(1)=1");
}

static void test_sqrtpd_precision(void) {
    xmm_t a, result;

    a.f64[0] = 2.0; a.f64[1] = 3.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabs(result.f64[0] - sqrt(2.0)) < 1e-15, "sqrtpd sqrt(2) precision");
    TEST_ASSERT(fabs(result.f64[1] - sqrt(3.0)) < 1e-15, "sqrtpd sqrt(3) precision");
}

static void test_sqrtpd_mem(void) {
    xmm_t result;
    xmm_t mem;

    mem.f64[0] = 25.0; mem.f64[1] = 100.0;
    __asm__ volatile (
        "sqrtpd %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "sqrtpd mem [0] sqrt(25)=5");
    TEST_ASSERT(result.f64[1] == 10.0, "sqrtpd mem [1] sqrt(100)=10");
}

static void test_sqrtpd_denormal(void) {
    xmm_t a, result;

    a.f64[0] = DBL_MIN / 4.0; a.f64[1] = DBL_MIN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabs(result.f64[0] - sqrt(DBL_MIN / 4.0)) < 1e-160,
                "sqrtpd sqrt(denormal)");
    TEST_ASSERT(fabs(result.f64[1] - sqrt(DBL_MIN)) < 1e-160,
                "sqrtpd sqrt(DBL_MIN)");
}

static void test_sqrtpd_xmm_xmm(void) {
    xmm_t a, b, result;

    a.f64[0] = 999.0; a.f64[1] = 888.0;
    b.f64[0] = 16.0;  b.f64[1] = 49.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "sqrtpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 4.0, "sqrtpd xmm,xmm [0] sqrt(16)=4");
    TEST_ASSERT(result.f64[1] == 7.0, "sqrtpd xmm,xmm [1] sqrt(49)=7");
}

int main(void) {
    TEST_START("SQRTPD instruction");
    test_sqrtpd_basic();
    test_sqrtpd_special();
    test_sqrtpd_precision();
    test_sqrtpd_mem();
    test_sqrtpd_denormal();
    test_sqrtpd_xmm_xmm();
    TEST_END();
}
