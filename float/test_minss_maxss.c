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
    TEST_ASSERT(IS_QNAN(result.f32[0]), "minss(5,QNaN)=QNaN (source returned)");

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
    TEST_ASSERT(IS_QNAN(result.f32[0]), "maxss(5,QNaN)=QNaN (source returned)");

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

#if ENABLE_MXCSR_CHECK
static void test_minmaxss_exact_bits_upper_lanes_and_snan(void) {
    xmm_t a = { .u32 = {
        UINT32_C(0x3f800000), UINT32_C(0x80000000),
        UINT32_C(0x7fc0abcd), UINT32_C(0x00000001)
    } };
    xmm_t b = { .u32 = {
        UINT32_C(0x7f812345), UINT32_C(0x11111111),
        UINT32_C(0x22222222), UINT32_C(0x33333333)
    } };
    xmm_t min_result, max_result, expected = a;
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr;
    expected.u32[0] = b.u32[0];

    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr));
    clean_mxcsr = old_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean_mxcsr));
    __asm__ volatile (
        "movaps %2, %%xmm0\n\tminss %3, %%xmm0\n\tmovaps %%xmm0, %0\n\t"
        "movaps %2, %%xmm0\n\tmaxss %3, %%xmm0\n\tmovaps %%xmm0, %1"
        : "=m"(min_result), "=m"(max_result) : "m"(a), "m"(b) : "xmm0"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr));
    TEST_ASSERT(memcmp(&min_result, &expected, sizeof(expected)) == 0,
                "MINSS exact SNaN source and destination upper lanes");
    TEST_ASSERT(memcmp(&max_result, &expected, sizeof(expected)) == 0,
                "MAXSS exact SNaN source and destination upper lanes");
    TEST_ASSERT(IS_SNAN(min_result.f32[0]) && IS_SNAN(max_result.f32[0]),
                "MINSS/MAXSS preserve selected SNaN classification");
    TEST_ASSERT(after_mxcsr & 1, "MINSS/MAXSS SNaN sets MXCSR invalid flag");

    a.u32[0] = UINT32_C(0x80000000);
    b.u32[0] = UINT32_C(0x00000000);
    expected = a;
    expected.u32[0] = b.u32[0];
    __asm__ volatile (
        "movaps %2, %%xmm0\n\tminss %3, %%xmm0\n\tmovaps %%xmm0, %0\n\t"
        "movaps %2, %%xmm0\n\tmaxss %3, %%xmm0\n\tmovaps %%xmm0, %1"
        : "=m"(min_result), "=m"(max_result) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(memcmp(&min_result, &expected, sizeof(expected)) == 0,
                "MINSS (-0,+0) selects exact +0 source and preserves upper lanes");
    TEST_ASSERT(memcmp(&max_result, &expected, sizeof(expected)) == 0,
                "MAXSS (-0,+0) selects exact +0 source and preserves upper lanes");
}
#endif

int main(void) {
    TEST_START("MINSS/MAXSS instructions");
    test_minss();
    test_minss_mem();
    test_maxss();
    test_maxss_mem();
#if ENABLE_MXCSR_CHECK
    test_minmaxss_exact_bits_upper_lanes_and_snan();
#endif
    TEST_END();
}
