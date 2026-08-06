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

#define TEST_PD_NANS(mnemonic, name) do { \
    const xmm_t q_a = { .f64 = {__builtin_nan("0x123456789abc"), 8.0} }; \
    const xmm_t q_b = { .f64 = {1.0, -__builtin_nan("0x23456789abcd")} }; \
    const xmm_t s_a = { .f64 = {__builtin_nans("0x123456789abc"), 8.0} }; \
    const xmm_t s_b = { .f64 = {1.0, -__builtin_nans("0x23456789abcd")} }; \
    xmm_t q_expected = q_a, s_expected = s_a; \
    xmm_t result; \
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr; \
    q_expected.u64[1] = q_b.u64[1]; \
    s_expected.u64[1] = s_b.u64[1]; \
    for (int lane = 0; lane < 2; lane++) \
        s_expected.u64[lane] |= UINT64_C(1) << 51; \
    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr)); \
    clean_mxcsr = (old_mxcsr | 0x80u) & ~0x3fu; \
    __asm__ volatile ( \
        "ldmxcsr %[clean]\n\tmovapd %[lhs], %%xmm0\n\t" \
        mnemonic " %[rhs], %%xmm0\n\tmovapd %%xmm0, %[result]\n\tstmxcsr %[after]" \
        : [result] "=m"(result), [after] "=m"(after_mxcsr) \
        : [clean] "m"(clean_mxcsr), [lhs] "m"(q_a), [rhs] "m"(q_b) \
        : "xmm0", "memory" \
    ); \
    TEST_ASSERT(memcmp(&result, &q_expected, sizeof(result)) == 0, \
                name " preserves qNaN sign and payload in every lane"); \
    TEST_ASSERT(IS_QNAN(result.f64[0]) && IS_QNAN(result.f64[1]), \
                name " qNaN results remain qNaNs"); \
    TEST_ASSERT((after_mxcsr & 1u) == 0, name " qNaN leaves MXCSR invalid clear"); \
    __asm__ volatile ( \
        "ldmxcsr %[clean]\n\tmovapd %[lhs], %%xmm0\n\t" \
        mnemonic " %[rhs], %%xmm0\n\tmovapd %%xmm0, %[result]\n\tstmxcsr %[after]" \
        : [result] "=m"(result), [after] "=m"(after_mxcsr) \
        : [clean] "m"(clean_mxcsr), [lhs] "m"(s_a), [rhs] "m"(s_b) \
        : "xmm0", "memory" \
    ); \
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr) : "memory"); \
    TEST_ASSERT(memcmp(&result, &s_expected, sizeof(result)) == 0, \
                name " quiets sNaN and preserves sign/payload in every lane"); \
    TEST_ASSERT(IS_QNAN(result.f64[0]) && IS_QNAN(result.f64[1]) && \
                !IS_SNAN(result.f64[0]) && !IS_SNAN(result.f64[1]), \
                name " sNaN results are qNaNs"); \
    TEST_ASSERT(after_mxcsr & 1u, name " sNaN sets MXCSR invalid"); \
} while (0)

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
    TEST_ASSERT(IS_QNAN(result.f64[0]), "mulpd inf*0=QNaN");
    TEST_ASSERT(IS_QNAN(result.f64[1]), "mulpd QNaN*42=QNaN");

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
    TEST_ASSERT(IS_QNAN(result.f64[1]), "divpd 0/0=QNaN");

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
    TEST_ASSERT(IS_QNAN(result.f64[0]), "divpd inf/inf=QNaN");
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

#if ENABLE_MXCSR_CHECK
static void test_mulpd_divpd_nan_bits(void) {
    TEST_PD_NANS("mulpd", "MULPD");
    TEST_PD_NANS("divpd", "DIVPD");
}
#endif

int main(void) {
    TEST_START("MULPD/DIVPD instructions");
    test_mulpd_basic();
    test_mulpd_special();
    test_mulpd_mem();
    test_divpd_basic();
    test_divpd_special();
    test_divpd_mem();
#if ENABLE_MXCSR_CHECK
    test_mulpd_divpd_nan_bits();
#endif
    TEST_END();
}
