/*
 * test_sqrtss.c - Test SSE SQRTSS/RCPSS/RSQRTSS instructions
 *
 * SQRTSS: Scalar single-precision square root.
 * RCPSS: Scalar single-precision reciprocal approximation (1/x, ~12-bit precision).
 * RSQRTSS: Scalar single-precision reciprocal square root approximation (1/sqrt(x), ~12-bit).
 * Upper 96 bits of destination are preserved (SQRTSS/RCPSS/RSQRTSS).
 *
 * Compile: gcc -o test_sqrtss float/test_sqrtss.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_sqrtss_basic(void) {
    xmm_t a, result;

    /* sqrt(4) = 2 */
    a.f32[0] = 4.0f; a.f32[1] = 11.0f; a.f32[2] = 22.0f; a.f32[3] = 33.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "sqrtss(4)=2: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 11.0f, "sqrtss upper[1] preserved");
    TEST_ASSERT(result.f32[2] == 22.0f, "sqrtss upper[2] preserved");
    TEST_ASSERT(result.f32[3] == 33.0f, "sqrtss upper[3] preserved");

    /* sqrt(0) = 0 */
    a.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "sqrtss(0)=0");

    /* sqrt(1) = 1 */
    a.f32[0] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 1.0f, "sqrtss(1)=1");

    /* sqrt(inf) = inf */
    a.f32[0] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] > 0, "sqrtss(inf)=inf");

    /* sqrt(-1) = NaN */
    a.f32[0] = -1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isnan(result.f32[0]), "sqrtss(-1)=NaN");

    /* sqrt(NaN) = NaN */
    a.f32[0] = NAN;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isnan(result.f32[0]), "sqrtss(NaN)=NaN");

    /* sqrt(2) ~ 1.41421 */
    a.f32[0] = 2.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - sqrtf(2.0f)) < 1e-6f,
                "sqrtss(2) ~ 1.41421: got %f", result.f32[0]);
}

static void test_sqrtss_mem(void) {
    xmm_t a, result;
    float mem_val = 9.0f;

    a.f32[0] = 0.0f; a.f32[1] = 11.0f; a.f32[2] = 22.0f; a.f32[3] = 33.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtss %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 3.0f, "sqrtss xmm,mem sqrt(9)=3");
    TEST_ASSERT(result.f32[1] == 11.0f, "sqrtss xmm,mem upper preserved");
}

static void test_rcpss(void) {
    xmm_t a, result;

    /* 1/1 ~ 1.0 */
    a.f32[0] = 1.0f; a.f32[1] = 11.0f; a.f32[2] = 22.0f; a.f32[3] = 33.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rcpss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 1.0f) < 0.001f,
                "rcpss(1) ~ 1.0: got %f", result.f32[0]);

    /* 1/2 ~ 0.5 */
    a.f32[0] = 2.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rcpss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 0.5f) < 0.001f,
                "rcpss(2) ~ 0.5: got %f", result.f32[0]);

    /* 1/4 ~ 0.25 */
    a.f32[0] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rcpss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 0.25f) < 0.001f,
                "rcpss(4) ~ 0.25: got %f", result.f32[0]);

    /* 1/inf = 0 */
    a.f32[0] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rcpss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "rcpss(inf)=0");

    /* Negative */
    a.f32[0] = -2.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rcpss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - (-0.5f)) < 0.001f,
                "rcpss(-2) ~ -0.5: got %f", result.f32[0]);
}

static void test_rsqrtss(void) {
    xmm_t a, result;

    /* 1/sqrt(1) ~ 1.0 */
    a.f32[0] = 1.0f; a.f32[1] = 11.0f; a.f32[2] = 22.0f; a.f32[3] = 33.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rsqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 1.0f) < 0.001f,
                "rsqrtss(1) ~ 1.0: got %f", result.f32[0]);

    /* 1/sqrt(4) ~ 0.5 */
    a.f32[0] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rsqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 0.5f) < 0.001f,
                "rsqrtss(4) ~ 0.5: got %f", result.f32[0]);

    /* 1/sqrt(inf) = 0 */
    a.f32[0] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rsqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "rsqrtss(inf)=0");

    /* 1/sqrt(0.25) ~ 2.0 */
    a.f32[0] = 0.25f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rsqrtss %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 2.0f) < 0.01f,
                "rsqrtss(0.25) ~ 2.0: got %f", result.f32[0]);
}

int main(void) {
    TEST_START("SQRTSS/RCPSS/RSQRTSS instructions");
    test_sqrtss_basic();
    test_sqrtss_mem();
    test_rcpss();
    test_rsqrtss();
    TEST_END();
}
