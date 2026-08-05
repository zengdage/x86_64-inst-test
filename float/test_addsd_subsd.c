/*
 * test_addsd_subsd.c - Test SSE2 ADDSD/SUBSD instructions
 *
 * ADDSD: Add scalar double-precision float (lowest 64 bits of XMM).
 * SUBSD: Subtract scalar double-precision float.
 * Upper 64 bits of destination are preserved.
 *
 * Compile: gcc -o test_addsd_subsd float/test_addsd_subsd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_addsd_basic(void) {
    xmm_t a, b, result;

    /* Basic addition */
    a.f64[0] = 1.5; a.f64[1] = 100.0;
    b.f64[0] = 2.5; b.f64[1] = 999.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 4.0, "addsd 1.5+2.5=4.0: got %f", result.f64[0]);
    TEST_ASSERT(result.f64[1] == 100.0, "addsd upper preserved: got %f", result.f64[1]);

    /* Inf + (-Inf) = NaN */
    a.f64[0] = INFINITY;
    b.f64[0] = -INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f64[0]), "addsd inf+(-inf)=QNaN");

    /* NaN propagation */
    a.f64[0] = NAN;
    b.f64[0] = 42.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f64[0]), "addsd QNaN+42=QNaN");

    /* Zero + Zero */
    a.f64[0] = 0.0;
    b.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 0.0, "addsd 0+0=0");

    /* Denormal */
    a.f64[0] = DBL_MIN / 2.0;
    b.f64[0] = DBL_MIN / 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == DBL_MIN, "addsd denorm+denorm=DBL_MIN");
}

static void test_addsd_mem(void) {
    xmm_t a, result;
    double mem_val = 7.0;

    a.f64[0] = 3.0; a.f64[1] = 50.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "addsd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 10.0, "addsd xmm,mem 3+7=10");
    TEST_ASSERT(result.f64[1] == 50.0, "addsd xmm,mem upper preserved");
}

static void test_subsd_basic(void) {
    xmm_t a, b, result;

    /* Basic subtraction */
    a.f64[0] = 10.0; a.f64[1] = 200.0;
    b.f64[0] = 3.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "subsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 7.0, "subsd 10-3=7: got %f", result.f64[0]);
    TEST_ASSERT(result.f64[1] == 200.0, "subsd upper preserved");

    /* x - x = 0 */
    a.f64[0] = 42.0;
    b.f64[0] = 42.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "subsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 0.0, "subsd x-x=0");

    /* Inf - Inf = NaN */
    a.f64[0] = INFINITY;
    b.f64[0] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "subsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f64[0]), "subsd inf-inf=QNaN");

    /* Negative result */
    a.f64[0] = 3.0;
    b.f64[0] = 10.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "subsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == -7.0, "subsd 3-10=-7");
}

static void test_subsd_mem(void) {
    xmm_t a, result;
    double mem_val = 2.0;

    a.f64[0] = 10.0; a.f64[1] = 50.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "subsd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 8.0, "subsd xmm,mem 10-2=8");
    TEST_ASSERT(result.f64[1] == 50.0, "subsd xmm,mem upper preserved");
}

int main(void) {
    TEST_START("ADDSD/SUBSD instructions");
    test_addsd_basic();
    test_addsd_mem();
    test_subsd_basic();
    test_subsd_mem();
    TEST_END();
}
