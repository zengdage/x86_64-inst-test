/*
 * test_minpd_maxpd.c - Test SSE2 MINPD/MAXPD instructions
 *
 * MINPD: Packed double-precision minimum (2 x 64-bit parallel).
 * MAXPD: Packed double-precision maximum.
 * If either operand is NaN, the second operand (source) is returned.
 *
 * Compile: gcc -o test_minpd_maxpd float/test_minpd_maxpd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_minpd_basic(void) {
    xmm_t a, b, result;

    a.f64[0] = 1.0; a.f64[1] = 5.0;
    b.f64[0] = 4.0; b.f64[1] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 1.0, "minpd [0] min(1,4)=1");
    TEST_ASSERT(result.f64[1] == 2.0, "minpd [1] min(5,2)=2");
}

static void test_minpd_special(void) {
    xmm_t a, b, result;

    /* NaN handling */
    a.f64[0] = NAN; a.f64[1] = 5.0;
    b.f64[0] = 5.0; b.f64[1] = NAN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "minpd(NaN,5)=5");
    TEST_ASSERT(isnan(result.f64[1]), "minpd(5,NaN)=NaN");

    /* -Inf and +0 vs -0 */
    a.f64[0] = -INFINITY; a.f64[1] = 0.0;
    b.f64[0] = FLT_MAX;   b.f64[1] = -0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "minpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f64[0]) && result.f64[0] < 0, "minpd(-inf,MAX)=-inf");
    TEST_ASSERT(result.f64[1] == 0.0 && signbit(result.f64[1]),
                "minpd(+0,-0)=-0 (source returned)");
}

static void test_minpd_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f64[0] = 10.0; a.f64[1] = 1.0;
    mem.f64[0] = 1.0; mem.f64[1] = 10.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "minpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 1.0, "minpd xmm,mem [0] min(10,1)=1");
    TEST_ASSERT(result.f64[1] == 1.0, "minpd xmm,mem [1] min(1,10)=1");
}

static void test_maxpd_basic(void) {
    xmm_t a, b, result;

    a.f64[0] = 1.0; a.f64[1] = 5.0;
    b.f64[0] = 4.0; b.f64[1] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 4.0, "maxpd [0] max(1,4)=4");
    TEST_ASSERT(result.f64[1] == 5.0, "maxpd [1] max(5,2)=5");
}

static void test_maxpd_special(void) {
    xmm_t a, b, result;

    a.f64[0] = NAN; a.f64[1] = -10.0;
    b.f64[0] = 5.0; b.f64[1] = -20.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "maxpd(NaN,5)=5");
    TEST_ASSERT(result.f64[1] == -10.0, "maxpd(-10,-20)=-10");

    /* Infinity */
    a.f64[0] = INFINITY; a.f64[1] = DBL_MIN / 2.0;
    b.f64[0] = DBL_MAX;  b.f64[1] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "maxpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(result.f64[0]) && result.f64[0] > 0, "maxpd(inf,DBL_MAX)=inf");
    TEST_ASSERT(result.f64[1] == DBL_MIN / 2.0, "maxpd(denorm,0)=denorm");
}

static void test_maxpd_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f64[0] = 10.0; a.f64[1] = 1.0;
    mem.f64[0] = 1.0; mem.f64[1] = 10.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "maxpd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 10.0, "maxpd xmm,mem [0] max(10,1)=10");
    TEST_ASSERT(result.f64[1] == 10.0, "maxpd xmm,mem [1] max(1,10)=10");
}

int main(void) {
    TEST_START("MINPD/MAXPD instructions");
    test_minpd_basic();
    test_minpd_special();
    test_minpd_mem();
    test_maxpd_basic();
    test_maxpd_special();
    test_maxpd_mem();
    TEST_END();
}
