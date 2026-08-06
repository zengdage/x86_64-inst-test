/*
 * test_addpd_subpd.c - Test SSE2 ADDPD/SUBPD instructions
 *
 * ADDPD: Add packed double-precision floats (2 x 64-bit parallel).
 * SUBPD: Subtract packed double-precision floats.
 *
 * Compile: gcc -o test_addpd_subpd float/test_addpd_subpd.c -O0 -lm
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

static void test_addpd_basic(void) {
    xmm_t a, b, result;

    a.f64[0] = 1.5; a.f64[1] = 2.5;
    b.f64[0] = 3.5; b.f64[1] = 4.5;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "addpd [0] 1.5+3.5=5: got %f", result.f64[0]);
    TEST_ASSERT(result.f64[1] == 7.0, "addpd [1] 2.5+4.5=7: got %f", result.f64[1]);
}

static void test_addpd_special(void) {
    xmm_t a, b, result;

    /* Inf + (-Inf) = NaN; NaN + x = NaN */
    a.f64[0] = INFINITY; a.f64[1] = NAN;
    b.f64[0] = -INFINITY; b.f64[1] = 42.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f64[0]), "addpd inf+(-inf)=QNaN");
    TEST_ASSERT(IS_QNAN(result.f64[1]), "addpd QNaN+42=QNaN");

    /* Zero */
    a.f64[0] = 0.0; a.f64[1] = -0.0;
    b.f64[0] = 0.0; b.f64[1] = -0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 0.0, "addpd 0+0=0");
    TEST_ASSERT(result.f64[1] == 0.0 && signbit(result.f64[1]), "addpd -0+(-0)=-0");
}

static void test_addpd_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f64[0] = 10.0; a.f64[1] = 20.0;
    mem.f64[0] = 1.0; mem.f64[1] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "addpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 11.0, "addpd xmm,mem [0] 10+1=11");
    TEST_ASSERT(result.f64[1] == 22.0, "addpd xmm,mem [1] 20+2=22");
}

static void test_subpd_basic(void) {
    xmm_t a, b, result;

    a.f64[0] = 10.0; a.f64[1] = 20.0;
    b.f64[0] = 3.0;  b.f64[1] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "subpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 7.0, "subpd [0] 10-3=7");
    TEST_ASSERT(result.f64[1] == 15.0, "subpd [1] 20-5=15");
}

static void test_subpd_special(void) {
    xmm_t a, b, result;

    a.f64[0] = 5.0; a.f64[1] = INFINITY;
    b.f64[0] = 5.0; b.f64[1] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "subpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 0.0, "subpd x-x=0");
    TEST_ASSERT(IS_QNAN(result.f64[1]), "subpd inf-inf=QNaN");
}

static void test_subpd_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f64[0] = 100.0; a.f64[1] = 200.0;
    mem.f64[0] = 1.0; mem.f64[1] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "subpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 99.0, "subpd xmm,mem [0] 100-1=99");
    TEST_ASSERT(result.f64[1] == 198.0, "subpd xmm,mem [1] 200-2=198");
}

static void test_addpd_denormal(void) {
    xmm_t a, b, result;

    double d = DBL_MIN / 2.0;
    a.f64[0] = d; a.f64[1] = d;
    b.f64[0] = d; b.f64[1] = d;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == DBL_MIN, "addpd denorm+denorm=DBL_MIN [0]");
    TEST_ASSERT(result.f64[1] == DBL_MIN, "addpd denorm+denorm=DBL_MIN [1]");
}

#if ENABLE_MXCSR_CHECK
static void test_addpd_subpd_nan_bits(void) {
    TEST_PD_NANS("addpd", "ADDPD");
    TEST_PD_NANS("subpd", "SUBPD");
}
#endif

int main(void) {
    TEST_START("ADDPD/SUBPD instructions");
    test_addpd_basic();
    test_addpd_special();
    test_addpd_mem();
    test_subpd_basic();
    test_subpd_special();
    test_subpd_mem();
    test_addpd_denormal();
#if ENABLE_MXCSR_CHECK
    test_addpd_subpd_nan_bits();
#endif
    TEST_END();
}
