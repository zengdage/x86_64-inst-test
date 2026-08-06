/*
 * test_sqrtps.c - Test SSE SQRTPS/RCPPS/RSQRTPS instructions
 *
 * SQRTPS: Packed single-precision square root (4 x 32-bit parallel).
 * RCPPS: Packed single-precision reciprocal approximation (~12-bit).
 * RSQRTPS: Packed single-precision reciprocal square root approximation (~12-bit).
 *
 * Compile: gcc -o test_sqrtps float/test_sqrtps.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_sqrtps_basic(void) {
    xmm_t a, result;

    a.f32[0] = 4.0f; a.f32[1] = 9.0f; a.f32[2] = 16.0f; a.f32[3] = 25.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "sqrtps [0] sqrt(4)=2");
    TEST_ASSERT(result.f32[1] == 3.0f, "sqrtps [1] sqrt(9)=3");
    TEST_ASSERT(result.f32[2] == 4.0f, "sqrtps [2] sqrt(16)=4");
    TEST_ASSERT(result.f32[3] == 5.0f, "sqrtps [3] sqrt(25)=5");
}

static void test_sqrtps_special(void) {
    xmm_t a, result;

    a.f32[0] = 0.0f; a.f32[1] = 1.0f; a.f32[2] = INFINITY; a.f32[3] = -1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "sqrtps sqrt(0)=0");
    TEST_ASSERT(result.f32[1] == 1.0f, "sqrtps sqrt(1)=1");
    TEST_ASSERT(isinf(result.f32[2]) && result.f32[2] > 0, "sqrtps sqrt(inf)=inf");
    TEST_ASSERT(IS_QNAN(result.f32[3]), "sqrtps sqrt(-1)=QNaN");
}

static void test_sqrtps_nan(void) {
    xmm_t a, result;

    a.f32[0] = NAN; a.f32[1] = 2.0f; a.f32[2] = -0.0f; a.f32[3] = FLT_MAX;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "sqrtps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(IS_QNAN(result.f32[0]), "sqrtps sqrt(QNaN)=QNaN");
    TEST_ASSERT(fabsf(result.f32[1] - sqrtf(2.0f)) < 1e-6f, "sqrtps sqrt(2)");
    TEST_ASSERT(result.f32[2] == 0.0f && signbit(result.f32[2]), "sqrtps sqrt(-0)=-0");
    TEST_ASSERT(fabsf(result.f32[3] - sqrtf(FLT_MAX)) < 1e30f, "sqrtps sqrt(FLT_MAX)");
}

static void test_sqrtps_mem(void) {
    xmm_t result;
    xmm_t mem;

    mem.f32[0] = 1.0f; mem.f32[1] = 4.0f; mem.f32[2] = 9.0f; mem.f32[3] = 16.0f;
    __asm__ volatile (
        "sqrtps %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 1.0f, "sqrtps mem [0] sqrt(1)=1");
    TEST_ASSERT(result.f32[1] == 2.0f, "sqrtps mem [1] sqrt(4)=2");
    TEST_ASSERT(result.f32[2] == 3.0f, "sqrtps mem [2] sqrt(9)=3");
    TEST_ASSERT(result.f32[3] == 4.0f, "sqrtps mem [3] sqrt(16)=4");
}

static void test_rcpps(void) {
    xmm_t a, result;

    a.f32[0] = 1.0f; a.f32[1] = 2.0f; a.f32[2] = 4.0f; a.f32[3] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rcpps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 1.0f) < 0.001f, "rcpps 1/1 ~ 1.0: got %f", result.f32[0]);
    TEST_ASSERT(fabsf(result.f32[1] - 0.5f) < 0.001f, "rcpps 1/2 ~ 0.5: got %f", result.f32[1]);
    TEST_ASSERT(fabsf(result.f32[2] - 0.25f) < 0.001f, "rcpps 1/4 ~ 0.25: got %f", result.f32[2]);
    TEST_ASSERT(result.f32[3] == 0.0f, "rcpps 1/inf=0");
}

static void test_rcpps_negative(void) {
    xmm_t a, result;

    a.f32[0] = -1.0f; a.f32[1] = -2.0f; a.f32[2] = -4.0f; a.f32[3] = -INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rcpps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - (-1.0f)) < 0.001f, "rcpps 1/(-1) ~ -1");
    TEST_ASSERT(fabsf(result.f32[1] - (-0.5f)) < 0.001f, "rcpps 1/(-2) ~ -0.5");
    TEST_ASSERT(result.f32[3] == -0.0f, "rcpps 1/(-inf)=-0");
}

static void test_rsqrtps(void) {
    xmm_t a, result;

    a.f32[0] = 1.0f; a.f32[1] = 4.0f; a.f32[2] = 0.25f; a.f32[3] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "rsqrtps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(fabsf(result.f32[0] - 1.0f) < 0.001f,
                "rsqrtps 1/sqrt(1) ~ 1.0: got %f", result.f32[0]);
    TEST_ASSERT(fabsf(result.f32[1] - 0.5f) < 0.001f,
                "rsqrtps 1/sqrt(4) ~ 0.5: got %f", result.f32[1]);
    TEST_ASSERT(fabsf(result.f32[2] - 2.0f) < 0.01f,
                "rsqrtps 1/sqrt(0.25) ~ 2.0: got %f", result.f32[2]);
    TEST_ASSERT(result.f32[3] == 0.0f, "rsqrtps 1/sqrt(inf)=0");
}

#if ENABLE_MXCSR_CHECK
static void test_sqrtps_exact_nan_bits_and_invalid(void) {
    xmm_t input = { .u32 = {
        UINT32_C(0x80000000), UINT32_C(0x7f800000),
        UINT32_C(0x7fc54321), UINT32_C(0x7f812345)
    } };
    xmm_t expected = { .u32 = {
        UINT32_C(0x80000000), UINT32_C(0x7f800000),
        UINT32_C(0x7fc54321), UINT32_C(0x7fc12345)
    } };
    xmm_t result;
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr;

    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr));
    clean_mxcsr = old_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean_mxcsr));
    __asm__ volatile (
        "sqrtps %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(result) : "m"(input) : "xmm0"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr));
    TEST_ASSERT(memcmp(&result, &expected, sizeof(expected)) == 0,
                "SQRTPS preserves -0/QNaN payload and quiets SNaN exactly");
    TEST_ASSERT(IS_QNAN(result.f32[2]) && IS_QNAN(result.f32[3]) &&
                !IS_SNAN(result.f32[3]), "SQRTPS NaN results are QNaNs");
    TEST_ASSERT(after_mxcsr & 1, "SQRTPS SNaN sets MXCSR invalid flag");
}
#endif

int main(void) {
    TEST_START("SQRTPS/RCPPS/RSQRTPS instructions");
    test_sqrtps_basic();
    test_sqrtps_special();
    test_sqrtps_nan();
    test_sqrtps_mem();
    test_rcpps();
    test_rcpps_negative();
    test_rsqrtps();
#if ENABLE_MXCSR_CHECK
    test_sqrtps_exact_nan_bits_and_invalid();
#endif
    TEST_END();
}
