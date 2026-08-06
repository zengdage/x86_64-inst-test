/*
 * test_mulps_divps.c - Test SSE MULPS/DIVPS instructions
 *
 * MULPS: Multiply packed single-precision floats (4 x 32-bit parallel).
 * DIVPS: Divide packed single-precision floats.
 *
 * Compile: gcc -o test_mulps_divps float/test_mulps_divps.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

#define TEST_PS_NANS(mnemonic, name) do { \
    const xmm_t q_a = { .f32 = { \
        __builtin_nanf("0x12345"), 1.0f, \
        -__builtin_nanf("0x34567"), 1.0f \
    } }; \
    const xmm_t q_b = { .f32 = { \
        1.0f, __builtin_nanf("0x23456"), \
        1.0f, -__builtin_nanf("0x45678") \
    } }; \
    const xmm_t s_a = { .f32 = { \
        __builtin_nansf("0x12345"), 1.0f, \
        -__builtin_nansf("0x34567"), 1.0f \
    } }; \
    const xmm_t s_b = { .f32 = { \
        1.0f, __builtin_nansf("0x23456"), \
        1.0f, -__builtin_nansf("0x45678") \
    } }; \
    xmm_t q_expected = q_a, s_expected = s_a; \
    xmm_t result; \
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr; \
    q_expected.u32[1] = q_b.u32[1]; \
    q_expected.u32[3] = q_b.u32[3]; \
    s_expected.u32[1] = s_b.u32[1]; \
    s_expected.u32[3] = s_b.u32[3]; \
    for (int lane = 0; lane < 4; lane++) \
        s_expected.u32[lane] |= UINT32_C(1) << 22; \
    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr)); \
    clean_mxcsr = (old_mxcsr | 0x80u) & ~0x3fu; \
    __asm__ volatile ( \
        "ldmxcsr %[clean]\n\tmovaps %[lhs], %%xmm0\n\t" \
        mnemonic " %[rhs], %%xmm0\n\tmovaps %%xmm0, %[result]\n\tstmxcsr %[after]" \
        : [result] "=m"(result), [after] "=m"(after_mxcsr) \
        : [clean] "m"(clean_mxcsr), [lhs] "m"(q_a), [rhs] "m"(q_b) \
        : "xmm0", "memory" \
    ); \
    TEST_ASSERT(memcmp(&result, &q_expected, sizeof(result)) == 0, \
                name " preserves qNaN sign and payload in every lane"); \
    TEST_ASSERT(IS_QNAN(result.f32[0]) && IS_QNAN(result.f32[1]) && \
                IS_QNAN(result.f32[2]) && IS_QNAN(result.f32[3]), \
                name " qNaN results remain qNaNs"); \
    TEST_ASSERT((after_mxcsr & 1u) == 0, name " qNaN leaves MXCSR invalid clear"); \
    __asm__ volatile ( \
        "ldmxcsr %[clean]\n\tmovaps %[lhs], %%xmm0\n\t" \
        mnemonic " %[rhs], %%xmm0\n\tmovaps %%xmm0, %[result]\n\tstmxcsr %[after]" \
        : [result] "=m"(result), [after] "=m"(after_mxcsr) \
        : [clean] "m"(clean_mxcsr), [lhs] "m"(s_a), [rhs] "m"(s_b) \
        : "xmm0", "memory" \
    ); \
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr) : "memory"); \
    TEST_ASSERT(memcmp(&result, &s_expected, sizeof(result)) == 0, \
                name " quiets sNaN and preserves sign/payload in every lane"); \
    TEST_ASSERT(IS_QNAN(result.f32[0]) && IS_QNAN(result.f32[1]) && \
                IS_QNAN(result.f32[2]) && IS_QNAN(result.f32[3]) && \
                !IS_SNAN(result.f32[0]) && !IS_SNAN(result.f32[1]) && \
                !IS_SNAN(result.f32[2]) && !IS_SNAN(result.f32[3]), \
                name " sNaN results are qNaNs"); \
    TEST_ASSERT(after_mxcsr & 1u, name " sNaN sets MXCSR invalid"); \
} while (0)

static void test_mulps_basic(void) {
    xmm_t a, b, result;

    a.f32[0] = 2.0f; a.f32[1] = 3.0f; a.f32[2] = 4.0f; a.f32[3] = 5.0f;
    b.f32[0] = 3.0f; b.f32[1] = 4.0f; b.f32[2] = 5.0f; b.f32[3] = 6.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 6.0f, "mulps [0] 2*3=6");
    TEST_ASSERT(result.f32[1] == 12.0f, "mulps [1] 3*4=12");
    TEST_ASSERT(result.f32[2] == 20.0f, "mulps [2] 4*5=20");
    TEST_ASSERT(result.f32[3] == 30.0f, "mulps [3] 5*6=30");
}

static void test_mulps_special(void) {
    xmm_t a, b, result;

    a.f32[0] = 0.0f; a.f32[1] = INFINITY; a.f32[2] = -3.0f; a.f32[3] = NAN;
    b.f32[0] = 42.0f; b.f32[1] = 0.0f; b.f32[2] = -5.0f; b.f32[3] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "mulps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "mulps 0*42=0");
    TEST_ASSERT(IS_QNAN(result.f32[1]), "mulps inf*0=QNaN");
    TEST_ASSERT(result.f32[2] == 15.0f, "mulps (-3)*(-5)=15");
    TEST_ASSERT(IS_QNAN(result.f32[3]), "mulps QNaN*1=QNaN");
}

static void test_mulps_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f32[0] = 2.0f; a.f32[1] = 4.0f; a.f32[2] = 6.0f; a.f32[3] = 8.0f;
    mem.f32[0] = 10.0f; mem.f32[1] = 10.0f; mem.f32[2] = 10.0f; mem.f32[3] = 10.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "mulps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 20.0f, "mulps xmm,mem [0] 2*10=20");
    TEST_ASSERT(result.f32[3] == 80.0f, "mulps xmm,mem [3] 8*10=80");
}

static void test_divps_basic(void) {
    xmm_t a, b, result;

    a.f32[0] = 10.0f; a.f32[1] = 20.0f; a.f32[2] = 30.0f; a.f32[3] = 40.0f;
    b.f32[0] = 2.0f;  b.f32[1] = 4.0f;  b.f32[2] = 5.0f;  b.f32[3] = 8.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "divps [0] 10/2=5");
    TEST_ASSERT(result.f32[1] == 5.0f, "divps [1] 20/4=5");
    TEST_ASSERT(result.f32[2] == 6.0f, "divps [2] 30/5=6");
    TEST_ASSERT(result.f32[3] == 5.0f, "divps [3] 40/8=5");
}

static void test_divps_special(void) {
    xmm_t a, b, result;

    a.f32[0] = 1.0f; a.f32[1] = 0.0f; a.f32[2] = INFINITY; a.f32[3] = NAN;
    b.f32[0] = 0.0f; b.f32[1] = 0.0f; b.f32[2] = INFINITY; b.f32[3] = 2.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "divps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] > 0, "divps 1/0=+inf");
    TEST_ASSERT(IS_QNAN(result.f32[1]), "divps 0/0=QNaN");
    TEST_ASSERT(IS_QNAN(result.f32[2]), "divps inf/inf=QNaN");
    TEST_ASSERT(IS_QNAN(result.f32[3]), "divps QNaN/2=QNaN");
}

static void test_divps_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f32[0] = 100.0f; a.f32[1] = 200.0f; a.f32[2] = 300.0f; a.f32[3] = 400.0f;
    mem.f32[0] = 10.0f; mem.f32[1] = 10.0f; mem.f32[2] = 10.0f; mem.f32[3] = 10.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "divps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 10.0f, "divps xmm,mem [0] 100/10=10");
    TEST_ASSERT(result.f32[3] == 40.0f, "divps xmm,mem [3] 400/10=40");
}

#if ENABLE_MXCSR_CHECK
static void test_mulps_divps_nan_bits(void) {
    TEST_PS_NANS("mulps", "MULPS");
    TEST_PS_NANS("divps", "DIVPS");
}
#endif

int main(void) {
    TEST_START("MULPS/DIVPS instructions");
    test_mulps_basic();
    test_mulps_special();
    test_mulps_mem();
    test_divps_basic();
    test_divps_special();
    test_divps_mem();
#if ENABLE_MXCSR_CHECK
    test_mulps_divps_nan_bits();
#endif
    TEST_END();
}
