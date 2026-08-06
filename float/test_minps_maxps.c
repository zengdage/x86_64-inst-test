/*
 * test_minps_maxps.c - Test SSE MINPS/MAXPS instructions
 *
 * MINPS: Packed single-precision minimum (4 x 32-bit parallel).
 * MAXPS: Packed single-precision maximum.
 * If either operand is NaN, the second operand (source) is returned.
 *
 * Compile: gcc -o test_minps_maxps float/test_minps_maxps.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_minps_basic(void) {
    xmm_t a, b, result;

    a.f32[0] = 1.0f; a.f32[1] = 5.0f; a.f32[2] = 3.0f; a.f32[3] = 8.0f;
    b.f32[0] = 4.0f; b.f32[1] = 2.0f; b.f32[2] = 3.0f; b.f32[3] = 9.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 1.0f, "minps [0] min(1,4)=1");
    TEST_ASSERT(result.f32[1] == 2.0f, "minps [1] min(5,2)=2");
    TEST_ASSERT(result.f32[2] == 3.0f, "minps [2] min(3,3)=3");
    TEST_ASSERT(result.f32[3] == 8.0f, "minps [3] min(8,9)=8");
}

static void test_minps_special(void) {
    xmm_t a, b, result;

    a.f32[0] = NAN; a.f32[1] = 5.0f; a.f32[2] = -INFINITY; a.f32[3] = 0.0f;
    b.f32[0] = 5.0f; b.f32[1] = NAN; b.f32[2] = FLT_MAX;   b.f32[3] = -0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "minps(NaN,5)=5 (source returned)");
    TEST_ASSERT(IS_QNAN(result.f32[1]), "minps(5,QNaN)=QNaN (source returned)");
    TEST_ASSERT(isinf(result.f32[2]) && result.f32[2] < 0, "minps(-inf,FLT_MAX)=-inf");
    TEST_ASSERT(result.f32[3] == 0.0f && signbit(result.f32[3]),
                "minps(+0,-0)=-0 (source returned)");
}

static void test_minps_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f32[0] = 10.0f; a.f32[1] = 1.0f; a.f32[2] = 5.0f; a.f32[3] = 3.0f;
    mem.f32[0] = 1.0f; mem.f32[1] = 10.0f; mem.f32[2] = 5.0f; mem.f32[3] = 30.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "minps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 1.0f, "minps xmm,mem [0] min(10,1)=1");
    TEST_ASSERT(result.f32[1] == 1.0f, "minps xmm,mem [1] min(1,10)=1");
    TEST_ASSERT(result.f32[2] == 5.0f, "minps xmm,mem [2] min(5,5)=5");
    TEST_ASSERT(result.f32[3] == 3.0f, "minps xmm,mem [3] min(3,30)=3");
}

static void test_maxps_basic(void) {
    xmm_t a, b, result;

    a.f32[0] = 1.0f; a.f32[1] = 5.0f; a.f32[2] = 3.0f; a.f32[3] = 8.0f;
    b.f32[0] = 4.0f; b.f32[1] = 2.0f; b.f32[2] = 3.0f; b.f32[3] = 9.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 4.0f, "maxps [0] max(1,4)=4");
    TEST_ASSERT(result.f32[1] == 5.0f, "maxps [1] max(5,2)=5");
    TEST_ASSERT(result.f32[2] == 3.0f, "maxps [2] max(3,3)=3");
    TEST_ASSERT(result.f32[3] == 9.0f, "maxps [3] max(8,9)=9");
}

static void test_maxps_special(void) {
    xmm_t a, b, result;

    a.f32[0] = NAN; a.f32[1] = 5.0f; a.f32[2] = INFINITY; a.f32[3] = -10.0f;
    b.f32[0] = 5.0f; b.f32[1] = NAN; b.f32[2] = FLT_MAX;  b.f32[3] = -20.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "maxps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 5.0f, "maxps(NaN,5)=5 (source returned)");
    TEST_ASSERT(IS_QNAN(result.f32[1]), "maxps(5,QNaN)=QNaN (source returned)");
    TEST_ASSERT(isinf(result.f32[2]) && result.f32[2] > 0, "maxps(inf,FLT_MAX)=inf");
    TEST_ASSERT(result.f32[3] == -10.0f, "maxps(-10,-20)=-10");
}

static void test_maxps_mem(void) {
    xmm_t a, result;
    xmm_t mem;

    a.f32[0] = 10.0f; a.f32[1] = 1.0f; a.f32[2] = 5.0f; a.f32[3] = 3.0f;
    mem.f32[0] = 1.0f; mem.f32[1] = 10.0f; mem.f32[2] = 5.0f; mem.f32[3] = 30.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "maxps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 10.0f, "maxps xmm,mem [0] max(10,1)=10");
    TEST_ASSERT(result.f32[1] == 10.0f, "maxps xmm,mem [1] max(1,10)=10");
    TEST_ASSERT(result.f32[2] == 5.0f, "maxps xmm,mem [2] max(5,5)=5");
    TEST_ASSERT(result.f32[3] == 30.0f, "maxps xmm,mem [3] max(3,30)=30");
}

static void test_minps_denormal(void) {
    xmm_t a, b, result;

    float d = FLT_MIN / 2.0f;
    a.f32[0] = d; a.f32[1] = 0.0f; a.f32[2] = d; a.f32[3] = -d;
    b.f32[0] = 0.0f; b.f32[1] = d; b.f32[2] = d; b.f32[3] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "minps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "minps min(denorm,0)=0");
    TEST_ASSERT(result.f32[1] == 0.0f, "minps min(0,denorm)=0");
    TEST_ASSERT(result.f32[3] == -d, "minps min(-denorm,0)=-denorm");
}

#if ENABLE_MXCSR_CHECK
static void test_minmaxps_exact_source_selection_and_snan(void) {
    xmm_t a = { .u32 = {
        UINT32_C(0x00000000), UINT32_C(0x80000000),
        UINT32_C(0x3f800000), UINT32_C(0x40000000)
    } };
    xmm_t b = { .u32 = {
        UINT32_C(0x80000000), UINT32_C(0x00000000),
        UINT32_C(0x7fc12345), UINT32_C(0x7f812345)
    } };
    xmm_t min_result, max_result;
    uint32_t old_mxcsr, clean_mxcsr, after_mxcsr;

    __asm__ volatile ("stmxcsr %0" : "=m"(old_mxcsr));
    clean_mxcsr = old_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean_mxcsr));
    __asm__ volatile (
        "movaps %2, %%xmm0\n\t"
        "minps %3, %%xmm0\n\t"
        "movaps %%xmm0, %0\n\t"
        "movaps %2, %%xmm0\n\t"
        "maxps %3, %%xmm0\n\t"
        "movaps %%xmm0, %1"
        : "=m"(min_result), "=m"(max_result)
        : "m"(a), "m"(b)
        : "xmm0"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile ("ldmxcsr %0" : : "m"(old_mxcsr));

    TEST_ASSERT(memcmp(&min_result, &b, sizeof(b)) == 0,
                "MINPS returns exact second operand for equal zeros and NaNs");
    TEST_ASSERT(memcmp(&max_result, &b, sizeof(b)) == 0,
                "MAXPS returns exact second operand for equal zeros and NaNs");
    TEST_ASSERT(IS_QNAN(min_result.f32[2]) && IS_QNAN(max_result.f32[2]) &&
                IS_SNAN(min_result.f32[3]) && IS_SNAN(max_result.f32[3]),
                "MINPS/MAXPS preserve selected QNaN/SNaN classification");
    TEST_ASSERT(after_mxcsr & 1, "MINPS/MAXPS SNaN sets MXCSR invalid flag");
}
#endif

int main(void) {
    TEST_START("MINPS/MAXPS instructions");
    test_minps_basic();
    test_minps_special();
    test_minps_mem();
    test_maxps_basic();
    test_maxps_special();
    test_maxps_mem();
    test_minps_denormal();
#if ENABLE_MXCSR_CHECK
    test_minmaxps_exact_source_selection_and_snan();
#endif
    TEST_END();
}
