/*
 * test_addss_subss.c - Test SSE ADDSS/SUBSS instructions
 *
 * ADDSS: Add scalar single-precision float (lowest 32 bits of XMM).
 * SUBSS: Subtract scalar single-precision float.
 * Upper 96 bits of destination are preserved.
 *
 * Compile: gcc -o test_addss_subss float/test_addss_subss.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_addss_basic(void) {
    xmm_t a, b, result;

    /* Basic addition */
    a.f32[0] = 1.5f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    b.f32[0] = 2.5f; b.f32[1] = 99.0f; b.f32[2] = 99.0f; b.f32[3] = 99.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 4.0f, "addss 1.5+2.5 = 4.0: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 10.0f, "addss upper[1] preserved: got %f", result.f32[1]);
    TEST_ASSERT(result.f32[2] == 20.0f, "addss upper[2] preserved: got %f", result.f32[2]);
    TEST_ASSERT(result.f32[3] == 30.0f, "addss upper[3] preserved: got %f", result.f32[3]);

    /* Add zero */
    a.f32[0] = 42.0f;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 42.0f, "addss x+0 = x");

    /* Inf + finite = Inf */
    a.f32[0] = INFINITY;
    b.f32[0] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] > 0, "addss inf+1 = inf");

    /* Inf + (-Inf) = NaN */
    a.f32[0] = INFINITY;
    b.f32[0] = -INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f32[0]), "addss inf+(-inf) = NaN");

    /* NaN + x = NaN */
    a.f32[0] = NAN;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f32[0]), "addss NaN+5 = NaN");

    /* Negative numbers */
    a.f32[0] = -10.0f;
    b.f32[0] = -20.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == -30.0f, "addss -10+(-20) = -30");
}

static void test_addss_mem(void) {
    xmm_t a, result;
    float mem_val = 7.0f;

    a.f32[0] = 3.0f; a.f32[1] = 11.0f; a.f32[2] = 22.0f; a.f32[3] = 33.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "addss %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 10.0f, "addss xmm,mem 3+7=10: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 11.0f, "addss xmm,mem upper preserved");
}

static void test_subss_basic(void) {
    xmm_t a, b, result;

    /* Basic subtraction */
    a.f32[0] = 10.0f; a.f32[1] = 1.0f; a.f32[2] = 2.0f; a.f32[3] = 3.0f;
    b.f32[0] = 3.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "subss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 7.0f, "subss 10-3 = 7: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 1.0f, "subss upper[1] preserved");

    /* Subtract same = 0 */
    a.f32[0] = 42.0f;
    b.f32[0] = 42.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "subss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "subss x-x = 0");

    /* Inf - Inf = NaN */
    a.f32[0] = INFINITY;
    b.f32[0] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "subss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f32[0]), "subss inf-inf = NaN");

    /* Denormal */
    a.f32[0] = FLT_MIN;
    b.f32[0] = FLT_MIN / 2.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "subss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == FLT_MIN / 2.0f, "subss denormal result");
}

static void test_subss_mem(void) {
    xmm_t a, result;
    float mem_val = 2.0f;

    a.f32[0] = 10.0f; a.f32[1] = 5.0f; a.f32[2] = 6.0f; a.f32[3] = 7.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "subss %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 8.0f, "subss xmm,mem 10-2=8");
    TEST_ASSERT(result.f32[1] == 5.0f, "subss xmm,mem upper preserved");
}

int main(void) {
    TEST_START("ADDSS/SUBSS instructions");
    test_addss_basic();
    test_addss_mem();
    test_subss_basic();
    test_subss_mem();
    TEST_END();
}
