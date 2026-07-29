/*
 * test_minss_maxss.c - Test SSE MINSS/MAXSS instructions
 *
 * MINSS: Return minimum of two scalar single-precision floats.
 * MAXSS: Return maximum of two scalar single-precision floats.
 * If either operand is NaN, the second operand (source) is returned.
 * Upper 96 bits of destination are preserved.
 *
 * Compile: gcc -o test_minss_maxss float/test_minss_maxss.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_minss(void) {
    xmm_t a, b, result;

    /* Basic min */
    a.f32[0] = 3.0f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 3.0f, "minss(3,5)=3: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 10.0f, "minss upper[1] preserved");

    /* Reversed */
    a.f32[0] = 5.0f;
    b.f32[0] = 3.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 3.0f, "minss(5,3)=3");

    /* Equal */
    a.f32[0] = 7.0f;
    b.f32[0] = 7.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 7.0f, "minss(7,7)=7");

    /* Negative values */
    a.f32[0] = -10.0f;
    b.f32[0] = -20.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == -20.0f, "minss(-10,-20)=-20");

    /* -Inf is minimum */
    a.f32[0] = 0.0f;
    b.f32[0] = -INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] < 0, "minss(0,-inf)=-inf");

    /* NaN in first operand: returns second operand */
    a.f32[0] = NAN;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "minss(NaN,5)=5 (source returned)");

    /* NaN in second operand: returns NaN */
    a.f32[0] = 5.0f;
    b.f32[0] = NAN;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f32[0]), "minss(5,NaN)=NaN (source returned)");

    /* +0 vs -0 */
    a.f32[0] = 0.0f;
    b.f32[0] = -0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    /* When operands are equal, source (second) is returned */
    TEST_ASSERT(result.f32[0] == 0.0f && signbit(result.f32[0]),
                "minss(+0,-0)=-0 (source returned)");
}

static void test_minss_mem(void) {
    xmm_t a, result;
    float mem_val = 2.0f;

    a.f32[0] = 5.0f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "minss %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "minss xmm,mem min(5,2)=2");
    TEST_ASSERT(result.f32[1] == 10.0f, "minss xmm,mem upper preserved");
}

static void test_maxss(void) {
    xmm_t a, b, result;

    /* Basic max */
    a.f32[0] = 3.0f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "maxss(3,5)=5: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 10.0f, "maxss upper[1] preserved");

    /* Reversed */
    a.f32[0] = 5.0f;
    b.f32[0] = 3.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "maxss(5,3)=5");

    /* Negative values */
    a.f32[0] = -10.0f;
    b.f32[0] = -20.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == -10.0f, "maxss(-10,-20)=-10");

    /* +Inf is maximum */
    a.f32[0] = 0.0f;
    b.f32[0] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] > 0, "maxss(0,inf)=inf");

    /* NaN in first operand: returns second */
    a.f32[0] = NAN;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "maxss(NaN,5)=5 (source returned)");

    /* NaN in second operand: returns NaN */
    a.f32[0] = 5.0f;
    b.f32[0] = NAN;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isnan(result.f32[0]), "maxss(5,NaN)=NaN (source returned)");

    /* Denormal */
    a.f32[0] = FLT_MIN / 2.0f;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxss %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == FLT_MIN / 2.0f, "maxss(denorm,0)=denorm");
}

static void test_maxss_mem(void) {
    xmm_t a, result;
    float mem_val = 100.0f;

    a.f32[0] = 5.0f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "maxss %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 100.0f, "maxss xmm,mem max(5,100)=100");
    TEST_ASSERT(result.f32[1] == 10.0f, "maxss xmm,mem upper preserved");
}

int main(void) {
    TEST_START("MINSS/MAXSS instructions");
    test_minss();
    test_minss_mem();
    test_maxss();
    test_maxss_mem();
    TEST_END();
}
