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
    TEST_ASSERT(IS_QNAN(result.f32[0]), "sqrtss(-1)=QNaN");

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
    TEST_ASSERT(IS_QNAN(result.f32[0]), "sqrtss(QNaN)=QNaN");

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

static void test_sqrtss_exact_snan_and_upper_lanes(void) {
    xmm_t dst = { .u32 = {
        UINT32_C(0xdeadbeef), UINT32_C(0x80000000),
        UINT32_C(0x7fc54321), UINT32_C(0x00000001)
    } };
    xmm_t src = { .u32 = {
        UINT32_C(0x7f812345), UINT32_C(0x11111111),
        UINT32_C(0x22222222), UINT32_C(0x33333333)
    } };
    xmm_t expected = dst, result;
    expected.u32[0] = UINT32_C(0x7fc12345);
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr;

    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr));
    clean_mxcsr = old_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean_mxcsr));
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "sqrtss %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(result) : "m"(dst), "m"(src) : "xmm0"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr));
    TEST_ASSERT(memcmp(&result, &expected, sizeof(expected)) == 0,
                "SQRTSS quiets SNaN and preserves exact destination upper lanes");
    TEST_ASSERT(IS_QNAN(result.f32[0]) && !IS_SNAN(result.f32[0]),
                "SQRTSS SNaN result is QNaN");
    TEST_ASSERT(after_mxcsr & 1, "SQRTSS SNaN sets MXCSR invalid flag");

    src.u32[0] = UINT32_C(0x80000000);
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "sqrtss %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(result) : "m"(dst), "m"(src) : "xmm0"
    );
    TEST_ASSERT(result.u32[0] == UINT32_C(0x80000000), "SQRTSS preserves -0 sign");
    TEST_ASSERT(memcmp(&result.u32[1], &dst.u32[1], 3 * sizeof(uint32_t)) == 0,
                "SQRTSS -0 case preserves all upper lanes");
}

int main(void) {
    TEST_START("SQRTSS/RCPSS/RSQRTSS instructions");
    test_sqrtss_basic();
    test_sqrtss_mem();
    test_rcpss();
    test_rsqrtss();
    test_sqrtss_exact_snan_and_upper_lanes();
    TEST_END();
}
