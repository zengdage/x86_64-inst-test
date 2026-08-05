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
    TEST_ASSERT(IS_QNAN(result.f64[1]), "minpd(5,QNaN)=QNaN");

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

static void test_minmaxpd_exact_source_selection_and_snan(void) {
    xmm_t a = { .u64 = {
        UINT64_C(0x0000000000000000), UINT64_C(0x3ff0000000000000)
    } };
    xmm_t b = { .u64 = {
        UINT64_C(0x8000000000000000), UINT64_C(0x7ff0000000001234)
    } };
    xmm_t min_result, max_result;
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr;

    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr));
    clean_mxcsr = old_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean_mxcsr));
    __asm__ volatile (
        "movapd %2, %%xmm0\n\t"
        "minpd %3, %%xmm0\n\t"
        "movapd %%xmm0, %0\n\t"
        "movapd %2, %%xmm0\n\t"
        "maxpd %3, %%xmm0\n\t"
        "movapd %%xmm0, %1"
        : "=m"(min_result), "=m"(max_result)
        : "m"(a), "m"(b)
        : "xmm0"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr));

    TEST_ASSERT(memcmp(&min_result, &b, sizeof(b)) == 0,
                "MINPD returns exact second operand for equal zero and SNaN");
    TEST_ASSERT(memcmp(&max_result, &b, sizeof(b)) == 0,
                "MAXPD returns exact second operand for equal zero and SNaN");
    TEST_ASSERT(IS_SNAN(min_result.f64[1]) && IS_SNAN(max_result.f64[1]),
                "MINPD/MAXPD preserve selected SNaN classification");
    TEST_ASSERT(after_mxcsr & 1, "MINPD/MAXPD SNaN sets MXCSR invalid flag");

    /* Reversing equal signed zeros must reverse the selected zero sign. */
    a.u64[0] = UINT64_C(0x8000000000000000);
    b.u64[0] = UINT64_C(0x0000000000000000);
    a.u64[1] = UINT64_C(0x4000000000000000);
    b.u64[1] = UINT64_C(0x7ff8123456789abc);
    __asm__ volatile (
        "movapd %2, %%xmm0\n\tminpd %3, %%xmm0\n\tmovapd %%xmm0, %0\n\t"
        "movapd %2, %%xmm0\n\tmaxpd %3, %%xmm0\n\tmovapd %%xmm0, %1"
        : "=m"(min_result), "=m"(max_result) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(memcmp(&min_result, &b, sizeof(b)) == 0,
                "MINPD exact +0 and QNaN source payload selection");
    TEST_ASSERT(memcmp(&max_result, &b, sizeof(b)) == 0,
                "MAXPD exact +0 and QNaN source payload selection");
}

int main(void) {
    TEST_START("MINPD/MAXPD instructions");
    test_minpd_basic();
    test_minpd_special();
    test_minpd_mem();
    test_maxpd_basic();
    test_maxpd_special();
    test_maxpd_mem();
    test_minmaxpd_exact_source_selection_and_snan();
    TEST_END();
}
