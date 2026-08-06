/*
 * test_roundss_sd.c - Test SSE4.1 ROUNDSS/ROUNDSD instructions
 *
 * ROUNDSS: Round scalar single-precision according to imm8 rounding mode.
 * ROUNDSD: Round scalar double-precision according to imm8 rounding mode.
 * imm8 bits [1:0]: 00=nearest, 01=floor, 10=ceil, 11=truncate
 * imm8 bit [2]: 1=use MXCSR rounding mode instead of imm8 bits [1:0]
 * Upper bits of destination are preserved.
 *
 * Compile: gcc -o test_roundss_sd float/test_roundss_sd.c -O0 -lm -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

#define ROUND_NEAREST 0
#define ROUND_FLOOR   1
#define ROUND_CEIL    2
#define ROUND_TRUNC   3

static void test_roundss_nearest(void) {
    xmm_t a, result;

    a.f32[0] = 2.5f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "roundss nearest 2.5 = 2 (banker's): got %f", result.f32[0]);
    TEST_ASSERT(result.f32[1] == 10.0f, "roundss upper[1] preserved");

    a.f32[0] = 3.5f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 4.0f, "roundss nearest 3.5 = 4 (banker's)");

    a.f32[0] = 2.3f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "roundss nearest 2.3 = 2");

    a.f32[0] = 2.7f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 3.0f, "roundss nearest 2.7 = 3");
}

static void test_roundss_floor(void) {
    xmm_t a, result;

    a.f32[0] = 2.7f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $1, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "roundss floor 2.7 = 2");

    a.f32[0] = -2.3f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $1, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == -3.0f, "roundss floor -2.3 = -3");
}

static void test_roundss_ceil(void) {
    xmm_t a, result;

    a.f32[0] = 2.3f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $2, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 3.0f, "roundss ceil 2.3 = 3");

    a.f32[0] = -2.7f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $2, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == -2.0f, "roundss ceil -2.7 = -2");
}

static void test_roundss_trunc(void) {
    xmm_t a, result;

    a.f32[0] = 2.9f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $3, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "roundss trunc 2.9 = 2");

    a.f32[0] = -2.9f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $3, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == -2.0f, "roundss trunc -2.9 = -2");
}

static void test_roundss_special(void) {
    xmm_t a, result;

    /* Inf stays Inf */
    a.f32[0] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isinf(result.f32[0]) && result.f32[0] > 0, "roundss inf=inf");

    /* NaN stays NaN */
    a.f32[0] = NAN;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(IS_QNAN(result.f32[0]), "roundss QNaN=QNaN");

    /* Zero stays zero */
    a.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "roundss 0=0");

    /* -0 stays -0 */
    a.f32[0] = -0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 0.0f && signbit(result.f32[0]), "roundss -0=-0");
}

static void test_roundss_mem(void) {
    xmm_t a, result;
    float mem_val = 7.7f;

    a.f32[0] = 0.0f; a.f32[1] = 10.0f; a.f32[2] = 20.0f; a.f32[3] = 30.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundss $1, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 7.0f, "roundss mem floor 7.7=7");
    TEST_ASSERT(result.f32[1] == 10.0f, "roundss mem upper preserved");
}

static void test_roundsd_nearest(void) {
    xmm_t a, result;

    a.f64[0] = 2.5; a.f64[1] = 100.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "roundsd nearest 2.5 = 2 (banker's)");
    TEST_ASSERT(result.f64[1] == 100.0, "roundsd upper preserved");

    a.f64[0] = 3.5;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 4.0, "roundsd nearest 3.5 = 4");
}

static void test_roundsd_floor(void) {
    xmm_t a, result;

    a.f64[0] = 2.7;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $1, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "roundsd floor 2.7 = 2");

    a.f64[0] = -2.3;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $1, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == -3.0, "roundsd floor -2.3 = -3");
}

static void test_roundsd_ceil(void) {
    xmm_t a, result;

    a.f64[0] = 2.3;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $2, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 3.0, "roundsd ceil 2.3 = 3");

    a.f64[0] = -2.7;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $2, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == -2.0, "roundsd ceil -2.7 = -2");
}

static void test_roundsd_trunc(void) {
    xmm_t a, result;

    a.f64[0] = 2.9;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $3, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "roundsd trunc 2.9 = 2");

    a.f64[0] = -2.9;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $3, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == -2.0, "roundsd trunc -2.9 = -2");
}

static void test_roundsd_special(void) {
    xmm_t a, result;

    a.f64[0] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isinf(result.f64[0]), "roundsd inf=inf");

    a.f64[0] = NAN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(IS_QNAN(result.f64[0]), "roundsd QNaN=QNaN");

    a.f64[0] = -0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 0.0 && signbit(result.f64[0]), "roundsd -0=-0");
}

static void test_roundsd_mem(void) {
    xmm_t a, result;
    double mem_val = 7.7;

    a.f64[0] = 0.0; a.f64[1] = 100.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundsd $1, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a), "m"(mem_val)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 7.0, "roundsd mem floor 7.7=7");
    TEST_ASSERT(result.f64[1] == 100.0, "roundsd mem upper preserved");
}

#if ENABLE_MXCSR_CHECK
static void test_scalar_round_mxcsr_and_exceptions(void) {
    xmm_t src1 = { .f32 = {99.0f, 11.0f, 22.0f, 33.0f} };
    xmm_t src2 = { .f32 = {1.9f, 1.0f, 2.0f, 3.0f} };
    xmm_t r;
    uint32_t saved, csr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    const float expected[4] = {2.0f, 1.0f, 2.0f, 1.0f};
    for (uint32_t mode = 0; mode < 4; mode++) {
        csr = (saved & ~(UINT32_C(3) << 13)) | (mode << 13);
        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        __asm__ volatile("movaps %1,%%xmm0\n\tmovaps %2,%%xmm1\n\troundss $4,%%xmm1,%%xmm0\n\tmovaps %%xmm0,%0"
            : "=m"(r) : "m"(src1), "m"(src2) : "xmm0", "xmm1");
        TEST_ASSERT(r.f32[0] == expected[mode], "roundss MXCSR mode %u", mode);
        TEST_ASSERT(r.f32[1] == 11.0f && r.f32[2] == 22.0f && r.f32[3] == 33.0f, "roundss upper lanes preserved mode %u", mode);
    }

    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("movaps %0,%%xmm0\n\troundss $0,%%xmm0,%%xmm0" : : "m"(src2) : "xmm0");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(csr & (1u << 5), "roundss unsuppressed precision flag");
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("movaps %0,%%xmm0\n\troundss $8,%%xmm0,%%xmm0" : : "m"(src2) : "xmm0");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(!(csr & (1u << 5)), "roundss imm bit3 suppresses precision flag");

    src2.u32[0] = UINT32_C(0x7f812345);
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile(
        "movaps %1, %%xmm0\n\t"
        "roundss $8, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(src1), "m"(src2) : "xmm0"
    );
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(r.u32[0] == UINT32_C(0x7fc12345),
                "roundss quiets SNaN with exact payload");
    TEST_ASSERT(IS_QNAN(r.f32[0]) && !IS_SNAN(r.f32[0]),
                "roundss SNaN result is QNaN");
    TEST_ASSERT(r.u32[1] == src1.u32[1] && r.u32[2] == src1.u32[2] &&
                r.u32[3] == src1.u32[3], "roundss SNaN preserves source1 upper lanes");
    TEST_ASSERT(csr & 1u, "roundss SNaN sets invalid despite imm bit3");

    xmm_t sd1 = { .u64 = {UINT64_C(0xdeadbeefdeadbeef),
                           UINT64_C(0xfff8123456789abc)} };
    xmm_t sd2 = { .f64 = {1.9, 0.0} };
    const double expected_sd[4] = {2.0, 1.0, 2.0, 1.0};
    for (uint32_t mode = 0; mode < 4; mode++) {
        csr = (saved & ~(UINT32_C(3) << 13)) | (mode << 13);
        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        __asm__ volatile(
            "movapd %1, %%xmm0\n\t"
            "roundsd $4, %2, %%xmm0\n\t"
            "movapd %%xmm0, %0"
            : "=m"(r) : "m"(sd1), "m"(sd2) : "xmm0"
        );
        TEST_ASSERT(r.f64[0] == expected_sd[mode], "roundsd MXCSR mode %u", mode);
        TEST_ASSERT(r.u64[1] == sd1.u64[1], "roundsd upper lane mode %u", mode);
    }

    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("roundsd $0, %0, %%xmm0" : : "m"(sd2) : "xmm0");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(csr & (1u << 5), "roundsd unsuppressed precision flag");
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("roundsd $8, %0, %%xmm0" : : "m"(sd2) : "xmm0");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(!(csr & (1u << 5)), "roundsd imm bit3 suppresses precision flag");

    sd2.u64[0] = UINT64_C(0x7ff0000000001234);
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile(
        "movapd %1, %%xmm0\n\t"
        "roundsd $8, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(sd1), "m"(sd2) : "xmm0"
    );
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(r.u64[0] == UINT64_C(0x7ff8000000001234),
                "roundsd quiets SNaN with exact payload");
    TEST_ASSERT(IS_QNAN(r.f64[0]) && !IS_SNAN(r.f64[0]),
                "roundsd SNaN result is QNaN");
    TEST_ASSERT(r.u64[1] == sd1.u64[1], "roundsd SNaN preserves source1 upper lane");
    TEST_ASSERT(csr & 1u, "roundsd SNaN sets invalid despite imm bit3");
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
}
#endif

int main(void) {
    TEST_START("ROUNDSS/ROUNDSD instructions (SSE4.1)");
    test_roundss_nearest();
    test_roundss_floor();
    test_roundss_ceil();
    test_roundss_trunc();
    test_roundss_special();
    test_roundss_mem();
    test_roundsd_nearest();
    test_roundsd_floor();
    test_roundsd_ceil();
    test_roundsd_trunc();
    test_roundsd_special();
    test_roundsd_mem();
#if ENABLE_MXCSR_CHECK
    test_scalar_round_mxcsr_and_exceptions();
#endif
    TEST_END();
}
