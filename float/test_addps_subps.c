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
    TEST_ASSERT(isnan(result.f32[3]), "addps NaN+5=NaN");
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
    TEST_ASSERT(isnan(result.f32[0]), "addps inf+(-inf)=NaN");
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
    TEST_ASSERT(isnan(result.f32[2]), "subps inf-inf=NaN");
    TEST_ASSERT(isnan(result.f32[3]), "subps NaN-1=NaN");
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
    TEST_END();
}
