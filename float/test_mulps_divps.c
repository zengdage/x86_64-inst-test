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
    TEST_ASSERT(isnan(result.f32[1]), "mulps inf*0=NaN");
    TEST_ASSERT(result.f32[2] == 15.0f, "mulps (-3)*(-5)=15");
    TEST_ASSERT(isnan(result.f32[3]), "mulps NaN*1=NaN");
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
    TEST_ASSERT(isnan(result.f32[1]), "divps 0/0=NaN");
    TEST_ASSERT(isnan(result.f32[2]), "divps inf/inf=NaN");
    TEST_ASSERT(isnan(result.f32[3]), "divps NaN/2=NaN");
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

int main(void) {
    TEST_START("MULPS/DIVPS instructions");
    test_mulps_basic();
    test_mulps_special();
    test_mulps_mem();
    test_divps_basic();
    test_divps_special();
    test_divps_mem();
    TEST_END();
}
