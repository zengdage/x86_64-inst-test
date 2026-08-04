/*
 * test_sqrtsd.c - Test SSE2 SQRTSD instruction
 *
 * SQRTSD: Scalar double-precision square root.
 * Upper 64 bits of destination are preserved.
 *
 * Compile: gcc -o test_sqrtsd float/test_sqrtsd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_sqrtsd_basic(void) {
    xmm_t a, result;

    /* sqrt(4) = 2 */
    a.f64[0] = 4.0; a.f64[1] = 100.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "sqrtsd(4)=2: got %f", result.f64[0]);
    TEST_ASSERT(result.f64[1] == 100.0, "sqrtsd upper preserved");

    /* sqrt(0) = 0 */
    a.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 0.0, "sqrtsd(0)=0");

    /* sqrt(1) = 1 */
    a.f64[0] = 1.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 1.0, "sqrtsd(1)=1");

    /* sqrt(2) */
    a.f64[0] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabs(result.f64[0] - sqrt(2.0)) < 1e-15,
                "sqrtsd(2) ~ 1.4142...: got %.15f", result.f64[0]);

    /* sqrt(inf) = inf */
    a.f64[0] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isinf(result.f64[0]) && result.f64[0] > 0, "sqrtsd(inf)=inf");

    /* sqrt(-1) = NaN */
    a.f64[0] = -1.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isnan(result.f64[0]), "sqrtsd(-1)=NaN");

    /* sqrt(NaN) = NaN */
    a.f64[0] = NAN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isnan(result.f64[0]), "sqrtsd(NaN)=NaN");

    /* sqrt(-0) = -0 */
    a.f64[0] = -0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 0.0 && signbit(result.f64[0]), "sqrtsd(-0)=-0");

    /* Denormal input */
    a.f64[0] = DBL_MIN / 4.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabs(result.f64[0] - sqrt(DBL_MIN / 4.0)) < 1e-160,
                "sqrtsd(denormal)");

    /* Large value */
    a.f64[0] = 1.0e300;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabs(result.f64[0] - sqrt(1.0e300)) < 1e135,
                "sqrtsd(1e300): got %.15e", result.f64[0]);
}

static void test_sqrtsd_mem(void) {
    xmm_t a, result;
    double mem_val = 25.0;

    a.f64[0] = 0.0; a.f64[1] = 100.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "sqrtsd %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 5.0, "sqrtsd xmm,mem sqrt(25)=5");
    TEST_ASSERT(result.f64[1] == 100.0, "sqrtsd xmm,mem upper preserved");
}

static void test_sqrtsd_xmm_xmm(void) {
    xmm_t a, b, result;

    a.f64[0] = 999.0; a.f64[1] = 100.0;
    b.f64[0] = 16.0; b.f64[1] = 200.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "sqrtsd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f64[0] == 4.0, "sqrtsd xmm,xmm sqrt(16)=4");
    TEST_ASSERT(result.f64[1] == 100.0, "sqrtsd xmm,xmm upper from dest preserved");
}

static void test_sqrtsd_exact_snan_and_upper_lane(void) {
    xmm_t dst = { .u64 = {
        UINT64_C(0xdeadbeefdeadbeef), UINT64_C(0xfff8123456789abc)
    } };
    xmm_t src = { .u64 = {
        UINT64_C(0x7ff0000000001234), UINT64_C(0x1111222233334444)
    } };
    xmm_t expected = dst, result;
    expected.u64[0] = UINT64_C(0x7ff8000000001234);
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr;

    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr));
    clean_mxcsr = old_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean_mxcsr));
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "sqrtsd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(result) : "m"(dst), "m"(src) : "xmm0"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr));
    TEST_ASSERT(memcmp(&result, &expected, sizeof(expected)) == 0,
                "SQRTSD quiets SNaN and preserves exact destination upper lane");
    TEST_ASSERT(after_mxcsr & 1, "SQRTSD SNaN sets MXCSR invalid flag");
}

int main(void) {
    TEST_START("SQRTSD instruction");
    test_sqrtsd_basic();
    test_sqrtsd_mem();
    test_sqrtsd_xmm_xmm();
    test_sqrtsd_exact_snan_and_upper_lane();
    TEST_END();
}
