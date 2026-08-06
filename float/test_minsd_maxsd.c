/*
 * test_minsd_maxsd.c - Test SSE2 MINSD/MAXSD instructions
 *
 * MINSD: Return minimum of two scalar double-precision floats.
 * MAXSD: Return maximum of two scalar double-precision floats.
 * If either operand is NaN, the second operand (source) is returned.
 * Upper 64 bits of destination are preserved.
 *
 * Compile: gcc -o test_minsd_maxsd float/test_minsd_maxsd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_minsd(void) {
    xmm_t a, b, result;

    /* Basic min */
    a.f64[0] = 3.0; a.f64[1] = 100.0;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 3.0, "minsd(3,5)=3");
    TEST_ASSERT(result.f64[1] == 100.0, "minsd upper preserved");

    /* Reversed */
    a.f64[0] = 5.0;
    b.f64[0] = 3.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 3.0, "minsd(5,3)=3");

    /* Negative */
    a.f64[0] = -10.0;
    b.f64[0] = -20.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == -20.0, "minsd(-10,-20)=-20");

    /* -Inf */
    a.f64[0] = 0.0;
    b.f64[0] = -INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f64[0]) && result.f64[0] < 0, "minsd(0,-inf)=-inf");

    /* NaN in first: returns second */
    a.f64[0] = NAN;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "minsd(NaN,5)=5");

    /* NaN in second: returns NaN */
    a.f64[0] = 5.0;
    b.f64[0] = NAN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f64[0]), "minsd(5,QNaN)=QNaN");

    /* +0 vs -0 */
    a.f64[0] = 0.0;
    b.f64[0] = -0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 0.0 && signbit(result.f64[0]),
                "minsd(+0,-0)=-0 (source returned)");
}

static void test_minsd_mem(void) {
    xmm_t a, result;
    double mem_val = 2.0;

    a.f64[0] = 5.0; a.f64[1] = 100.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "minsd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "minsd xmm,mem min(5,2)=2");
    TEST_ASSERT(result.f64[1] == 100.0, "minsd xmm,mem upper preserved");
}

static void test_maxsd(void) {
    xmm_t a, b, result;

    /* Basic max */
    a.f64[0] = 3.0; a.f64[1] = 100.0;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "maxsd(3,5)=5");
    TEST_ASSERT(result.f64[1] == 100.0, "maxsd upper preserved");

    /* +Inf */
    a.f64[0] = 0.0;
    b.f64[0] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f64[0]) && result.f64[0] > 0, "maxsd(0,inf)=inf");

    /* NaN in first: returns second */
    a.f64[0] = NAN;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "maxsd(NaN,5)=5");

    /* Negative */
    a.f64[0] = -10.0;
    b.f64[0] = -20.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == -10.0, "maxsd(-10,-20)=-10");

    /* Denormal vs 0 */
    a.f64[0] = DBL_MIN / 2.0;
    b.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == DBL_MIN / 2.0, "maxsd(denorm,0)=denorm");
}

static void test_maxsd_mem(void) {
    xmm_t a, result;
    double mem_val = 100.0;

    a.f64[0] = 5.0; a.f64[1] = 50.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "maxsd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 100.0, "maxsd xmm,mem max(5,100)=100");
    TEST_ASSERT(result.f64[1] == 50.0, "maxsd xmm,mem upper preserved");
}

#if ENABLE_MXCSR_CHECK
static void test_minmaxsd_exact_bits_upper_lane_and_snan(void) {
    xmm_t a = { .u64 = {
        UINT64_C(0x3ff0000000000000), UINT64_C(0xfff8123456789abc)
    } };
    xmm_t b = { .u64 = {
        UINT64_C(0x7ff0000000001234), UINT64_C(0x1111222233334444)
    } };
    xmm_t min_result, max_result, expected = a;
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr;
    expected.u64[0] = b.u64[0];

    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr));
    clean_mxcsr = old_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean_mxcsr));
    __asm__ volatile (
        "movapd %2, %%xmm0\n\tminsd %3, %%xmm0\n\tmovapd %%xmm0, %0\n\t"
        "movapd %2, %%xmm0\n\tmaxsd %3, %%xmm0\n\tmovapd %%xmm0, %1"
        : "=m"(min_result), "=m"(max_result) : "m"(a), "m"(b) : "xmm0"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr));
    TEST_ASSERT(memcmp(&min_result, &expected, sizeof(expected)) == 0,
                "MINSD exact SNaN source and destination upper lane");
    TEST_ASSERT(memcmp(&max_result, &expected, sizeof(expected)) == 0,
                "MAXSD exact SNaN source and destination upper lane");
    TEST_ASSERT(IS_SNAN(min_result.f64[0]) && IS_SNAN(max_result.f64[0]),
                "MINSD/MAXSD preserve selected SNaN classification");
    TEST_ASSERT(after_mxcsr & 1, "MINSD/MAXSD SNaN sets MXCSR invalid flag");

    a.u64[0] = UINT64_C(0x8000000000000000);
    b.u64[0] = UINT64_C(0x0000000000000000);
    expected = a;
    expected.u64[0] = b.u64[0];
    __asm__ volatile (
        "movapd %2, %%xmm0\n\tminsd %3, %%xmm0\n\tmovapd %%xmm0, %0\n\t"
        "movapd %2, %%xmm0\n\tmaxsd %3, %%xmm0\n\tmovapd %%xmm0, %1"
        : "=m"(min_result), "=m"(max_result) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(memcmp(&min_result, &expected, sizeof(expected)) == 0,
                "MINSD (-0,+0) selects exact +0 source and preserves upper lane");
    TEST_ASSERT(memcmp(&max_result, &expected, sizeof(expected)) == 0,
                "MAXSD (-0,+0) selects exact +0 source and preserves upper lane");
}
#endif

int main(void) {
    TEST_START("MINSD/MAXSD instructions");
    test_minsd();
    test_minsd_mem();
    test_maxsd();
    test_maxsd_mem();
#if ENABLE_MXCSR_CHECK
    test_minmaxsd_exact_bits_upper_lane_and_snan();
#endif
    TEST_END();
}
