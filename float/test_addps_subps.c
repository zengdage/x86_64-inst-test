/*
 * test_addps_subps.c - Test SSE ADDPS/SUBPS instructions
 *
 * ADDPS: Add packed single-precision floats (4 x 32-bit parallel).
 * SUBPS: Subtract packed single-precision floats.
 *
 * Compile: gcc -o test_addps_subps float/test_addps_subps.c -O0 -lm
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

static void test_addps_basic(void) {
    xmm_t a, b, result;

    a.f32[0] = 1.0f; a.f32[1] = 2.0f; a.f32[2] = 3.0f; a.f32[3] = 4.0f;
    b.f32[0] = 5.0f; b.f32[1] = 6.0f; b.f32[2] = 7.0f; b.f32[3] = 8.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 6.0f, "addps [0] 1+5=6: got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 8.0f, "addps [1] 2+6=8: got %f", result.f32[1]);
    TEST_ASSERT(result.f32[2] == 10.0f, "addps [2] 3+7=10: got %f", result.f32[2]);
    TEST_ASSERT(result.f32[3] == 12.0f, "addps [3] 4+8=12: got %f", result.f32[3]);
}

static void test_addps_special(void) {
    xmm_t a, b, result;

    /* Mixed: zero, neg, inf, NaN */
    a.f32[0] = 0.0f; a.f32[1] = -10.0f; a.f32[2] = INFINITY; a.f32[3] = NAN;
    b.f32[0] = 0.0f; b.f32[1] = -20.0f; b.f32[2] = 1.0f;    b.f32[3] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "addps 0+0=0");
    TEST_ASSERT(result.f32[1] == -30.0f, "addps -10+(-20)=-30");
    TEST_ASSERT(isinf(result.f32[2]) && result.f32[2] > 0, "addps inf+1=inf");
    TEST_ASSERT(IS_QNAN(result.f32[3]), "addps QNaN+5=QNaN");
}

static void test_addps_inf_nan(void) {
    xmm_t a, b, result;

    a.f32[0] = INFINITY; a.f32[1] = -INFINITY; a.f32[2] = INFINITY; a.f32[3] = 0.0f;
    b.f32[0] = -INFINITY; b.f32[1] = -INFINITY; b.f32[2] = INFINITY; b.f32[3] = -0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(result.f32[0]), "addps inf+(-inf)=QNaN");
    TEST_ASSERT(isinf(result.f32[1]) && result.f32[1] < 0, "addps -inf+(-inf)=-inf");
    TEST_ASSERT(isinf(result.f32[2]) && result.f32[2] > 0, "addps inf+inf=inf");
    TEST_ASSERT(result.f32[3] == 0.0f, "addps 0+(-0)=0");
}

static void test_addps_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f32[0] = 1.0f; a.f32[1] = 2.0f; a.f32[2] = 3.0f; a.f32[3] = 4.0f;
    mem.f32[0] = 10.0f; mem.f32[1] = 20.0f; mem.f32[2] = 30.0f; mem.f32[3] = 40.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "addps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 11.0f, "addps xmm,mem [0] 1+10=11");
    TEST_ASSERT(result.f32[1] == 22.0f, "addps xmm,mem [1] 2+20=22");
    TEST_ASSERT(result.f32[2] == 33.0f, "addps xmm,mem [2] 3+30=33");
    TEST_ASSERT(result.f32[3] == 44.0f, "addps xmm,mem [3] 4+40=44");
}

static void test_subps_basic(void) {
    xmm_t a, b, result;

    a.f32[0] = 10.0f; a.f32[1] = 20.0f; a.f32[2] = 30.0f; a.f32[3] = 40.0f;
    b.f32[0] = 1.0f;  b.f32[1] = 2.0f;  b.f32[2] = 3.0f;  b.f32[3] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "subps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 9.0f, "subps [0] 10-1=9");
    TEST_ASSERT(result.f32[1] == 18.0f, "subps [1] 20-2=18");
    TEST_ASSERT(result.f32[2] == 27.0f, "subps [2] 30-3=27");
    TEST_ASSERT(result.f32[3] == 36.0f, "subps [3] 40-4=36");
}

static void test_subps_special(void) {
    xmm_t a, b, result;

    a.f32[0] = 5.0f; a.f32[1] = 5.0f; a.f32[2] = INFINITY; a.f32[3] = NAN;
    b.f32[0] = 5.0f; b.f32[1] = -5.0f; b.f32[2] = INFINITY; b.f32[3] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "subps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "subps x-x=0");
    TEST_ASSERT(result.f32[1] == 10.0f, "subps 5-(-5)=10");
    TEST_ASSERT(IS_QNAN(result.f32[2]), "subps inf-inf=QNaN");
    TEST_ASSERT(IS_QNAN(result.f32[3]), "subps QNaN-1=QNaN");
}

static void test_subps_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f32[0] = 100.0f; a.f32[1] = 200.0f; a.f32[2] = 300.0f; a.f32[3] = 400.0f;
    mem.f32[0] = 1.0f; mem.f32[1] = 2.0f; mem.f32[2] = 3.0f; mem.f32[3] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "subps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 99.0f, "subps xmm,mem [0] 100-1=99");
    TEST_ASSERT(result.f32[3] == 396.0f, "subps xmm,mem [3] 400-4=396");
}

static void test_addps_denormal(void) {
    xmm_t a, b, result;

    float d = FLT_MIN / 2.0f;
    a.f32[0] = d; a.f32[1] = d; a.f32[2] = d; a.f32[3] = d;
    b.f32[0] = d; b.f32[1] = d; b.f32[2] = d; b.f32[3] = d;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == FLT_MIN, "addps denorm+denorm=FLT_MIN [0]");
    TEST_ASSERT(result.f32[3] == FLT_MIN, "addps denorm+denorm=FLT_MIN [3]");
}

static void test_addps_subps_nan_bits(void) {
    TEST_PS_NANS("addps", "ADDPS");
    TEST_PS_NANS("subps", "SUBPS");
}

int main(void) {
    TEST_START("ADDPS/SUBPS instructions");
    test_addps_basic();
    test_addps_special();
    test_addps_inf_nan();
    test_addps_mem();
    test_subps_basic();
    test_subps_special();
    test_subps_mem();
    test_addps_denormal();
    test_addps_subps_nan_bits();
    TEST_END();
}
