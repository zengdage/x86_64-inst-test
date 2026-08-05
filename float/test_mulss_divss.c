/*
 * test_mulss_divss.c - Test SSE MULSS/DIVSS instructions
 *
 * MULSS: Multiply scalar single-precision float (lowest 32 bits of XMM).
 * DIVSS: Divide scalar single-precision float.
 * Upper 96 bits of destination are preserved.
 *
 * Compile: gcc -o test_mulss_divss float/test_mulss_divss.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_mulss(void) {
    xmm_t a, b, result;

    /* Basic multiply */
    a.f32[0] = 3.0f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    b.f32[0] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 12.0f, "mulss 3*4=12: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 10.0f, "mulss upper[1] preserved");

    /* Multiply by zero */
    a.f32[0] = 42.0f;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "mulss x*0=0");

    /* Multiply by one */
    a.f32[0] = 42.0f;
    b.f32[0] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 42.0f, "mulss x*1=x");

    /* Inf * 0 = NaN */
    a.f32[0] = INFINITY;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f32[0]), "mulss inf*0=QNaN");

    /* Negative * Negative = Positive */
    a.f32[0] = -3.0f;
    b.f32[0] = -5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 15.0f, "mulss (-3)*(-5)=15");

    /* NaN * x = NaN */
    a.f32[0] = NAN;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f32[0]), "mulss QNaN*5=QNaN");
}

static void test_mulss_mem(void) {
    xmm_t a, result;
    float mem_val = 5.0f;

    a.f32[0] = 6.0f; a.f32[1] = 11.0f; a.f32[2] = 22.0f; a.f32[3] = 33.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "mulss %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 30.0f, "mulss xmm,mem 6*5=30");
    TEST_ASSERT(result.f32[1] == 11.0f, "mulss xmm,mem upper preserved");
}

static void test_divss(void) {
    xmm_t a, b, result;

    /* Basic division */
    a.f32[0] = 10.0f; a.f32[1] = 1.0f; a.f32[2] = 2.0f; a.f32[3] = 3.0f;
    b.f32[0] = 2.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "divss 10/2=5: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 1.0f, "divss upper[1] preserved");

    /* x / 0 = Inf */
    a.f32[0] = 1.0f;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] > 0, "divss 1/0=+inf");

    /* -x / 0 = -Inf */
    a.f32[0] = -1.0f;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] < 0, "divss -1/0=-inf");

    /* 0/0 = NaN */
    a.f32[0] = 0.0f;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f32[0]), "divss 0/0=QNaN");

    /* Inf / Inf = NaN */
    a.f32[0] = INFINITY;
    b.f32[0] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f32[0]), "divss inf/inf=QNaN");

    /* Denormal / 2 */
    a.f32[0] = FLT_MIN;
    b.f32[0] = 2.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == FLT_MIN / 2.0f, "divss FLT_MIN/2 = denormal");
}

static void test_divss_mem(void) {
    xmm_t a, result;
    float mem_val = 4.0f;

    a.f32[0] = 20.0f; a.f32[1] = 11.0f; a.f32[2] = 22.0f; a.f32[3] = 33.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "divss %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "divss xmm,mem 20/4=5");
    TEST_ASSERT(result.f32[1] == 11.0f, "divss xmm,mem upper preserved");
}

int main(void) {
    TEST_START("MULSS/DIVSS instructions");
    test_mulss();
    test_mulss_mem();
    test_divss();
    test_divss_mem();
    TEST_END();
}
