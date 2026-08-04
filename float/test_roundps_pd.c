/*
 * test_roundps_pd.c - Test SSE4.1 ROUNDPS/ROUNDPD instructions
 *
 * ROUNDPS: Round packed single-precision floats (4 x 32-bit) according to imm8.
 * ROUNDPD: Round packed double-precision floats (2 x 64-bit) according to imm8.
 * imm8 bits [1:0]: 00=nearest, 01=floor, 10=ceil, 11=truncate
 * imm8 bit [2]: 1=use MXCSR rounding mode instead of imm8 bits [1:0]
 *
 * Compile: gcc -o test_roundps_pd float/test_roundps_pd.c -O0 -lm -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_roundps_nearest(void) {
    xmm_t a, result;

    a.f32[0] = 2.5f; a.f32[1] = 3.5f; a.f32[2] = 2.3f; a.f32[3] = 2.7f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundps $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "roundps nearest [0] 2.5=2 (banker's)");
    TEST_ASSERT(result.f32[1] == 4.0f, "roundps nearest [1] 3.5=4 (banker's)");
    TEST_ASSERT(result.f32[2] == 2.0f, "roundps nearest [2] 2.3=2");
    TEST_ASSERT(result.f32[3] == 3.0f, "roundps nearest [3] 2.7=3");
}

static void test_roundps_floor(void) {
    xmm_t a, result;

    a.f32[0] = 2.7f; a.f32[1] = -2.3f; a.f32[2] = 0.1f; a.f32[3] = -0.1f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundps $1, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "roundps floor [0] 2.7=2");
    TEST_ASSERT(result.f32[1] == -3.0f, "roundps floor [1] -2.3=-3");
    TEST_ASSERT(result.f32[2] == 0.0f, "roundps floor [2] 0.1=0");
    TEST_ASSERT(result.f32[3] == -1.0f, "roundps floor [3] -0.1=-1");
}

static void test_roundps_ceil(void) {
    xmm_t a, result;

    a.f32[0] = 2.3f; a.f32[1] = -2.7f; a.f32[2] = 0.1f; a.f32[3] = -0.1f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundps $2, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 3.0f, "roundps ceil [0] 2.3=3");
    TEST_ASSERT(result.f32[1] == -2.0f, "roundps ceil [1] -2.7=-2");
    TEST_ASSERT(result.f32[2] == 1.0f, "roundps ceil [2] 0.1=1");
    TEST_ASSERT(result.f32[3] == 0.0f, "roundps ceil [3] -0.1=0");
}

static void test_roundps_trunc(void) {
    xmm_t a, result;

    a.f32[0] = 2.9f; a.f32[1] = -2.9f; a.f32[2] = 100.1f; a.f32[3] = -100.9f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundps $3, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 2.0f, "roundps trunc [0] 2.9=2");
    TEST_ASSERT(result.f32[1] == -2.0f, "roundps trunc [1] -2.9=-2");
    TEST_ASSERT(result.f32[2] == 100.0f, "roundps trunc [2] 100.1=100");
    TEST_ASSERT(result.f32[3] == -100.0f, "roundps trunc [3] -100.9=-100");
}

static void test_roundps_special(void) {
    xmm_t a, result;

    a.f32[0] = 0.0f; a.f32[1] = -0.0f; a.f32[2] = INFINITY; a.f32[3] = NAN;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "roundps $0, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 0.0f, "roundps 0=0");
    TEST_ASSERT(result.f32[1] == 0.0f && signbit(result.f32[1]), "roundps -0=-0");
    TEST_ASSERT(isinf(result.f32[2]), "roundps inf=inf");
    TEST_ASSERT(isnan(result.f32[3]), "roundps NaN=NaN");
}

static void test_roundps_mem(void) {
    xmm_t result;
    xmm_t mem;

    mem.f32[0] = 7.7f; mem.f32[1] = -3.2f; mem.f32[2] = 0.5f; mem.f32[3] = -0.5f;
    __asm__ volatile (
        "roundps $1, %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(result)
        : "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f32[0] == 7.0f, "roundps mem floor [0] 7.7=7");
    TEST_ASSERT(result.f32[1] == -4.0f, "roundps mem floor [1] -3.2=-4");
    TEST_ASSERT(result.f32[2] == 0.0f, "roundps mem floor [2] 0.5=0");
    TEST_ASSERT(result.f32[3] == -1.0f, "roundps mem floor [3] -0.5=-1");
}

static void test_roundpd_nearest(void) {
    xmm_t a, result;

    a.f64[0] = 2.5; a.f64[1] = 3.5;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundpd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "roundpd nearest [0] 2.5=2 (banker's)");
    TEST_ASSERT(result.f64[1] == 4.0, "roundpd nearest [1] 3.5=4 (banker's)");
}

static void test_roundpd_floor(void) {
    xmm_t a, result;

    a.f64[0] = 2.7; a.f64[1] = -2.3;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundpd $1, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "roundpd floor [0] 2.7=2");
    TEST_ASSERT(result.f64[1] == -3.0, "roundpd floor [1] -2.3=-3");
}

static void test_roundpd_ceil(void) {
    xmm_t a, result;

    a.f64[0] = 2.3; a.f64[1] = -2.7;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundpd $2, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 3.0, "roundpd ceil [0] 2.3=3");
    TEST_ASSERT(result.f64[1] == -2.0, "roundpd ceil [1] -2.7=-2");
}

static void test_roundpd_trunc(void) {
    xmm_t a, result;

    a.f64[0] = 2.9; a.f64[1] = -2.9;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundpd $3, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 2.0, "roundpd trunc [0] 2.9=2");
    TEST_ASSERT(result.f64[1] == -2.0, "roundpd trunc [1] -2.9=-2");
}

static void test_roundpd_special(void) {
    xmm_t a, result;

    a.f64[0] = INFINITY; a.f64[1] = NAN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundpd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(isinf(result.f64[0]), "roundpd inf=inf");
    TEST_ASSERT(isnan(result.f64[1]), "roundpd NaN=NaN");

    a.f64[0] = -0.0; a.f64[1] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "roundpd $0, %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(a)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 0.0 && signbit(result.f64[0]), "roundpd -0=-0");
    TEST_ASSERT(result.f64[1] == 0.0, "roundpd 0=0");
}

static void test_roundpd_mem(void) {
    xmm_t result;
    xmm_t mem;

    mem.f64[0] = 7.7; mem.f64[1] = -3.2;
    __asm__ volatile (
        "roundpd $1, %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(result)
        : "m"(mem)
        : "xmm0"
    );
    TEST_ASSERT(result.f64[0] == 7.0, "roundpd mem floor [0] 7.7=7");
    TEST_ASSERT(result.f64[1] == -4.0, "roundpd mem floor [1] -3.2=-4");
}

static void test_round_mxcsr_and_exceptions(void) {
    xmm_t a = { .f32 = {1.9f, -1.1f, 2.5f, -2.5f} }, r;
    uint32_t saved, csr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    const uint32_t modes[4] = {0u, 1u, 2u, 3u};
    const float expected[4][4] = {
        {2.0f,-1.0f,2.0f,-2.0f}, {1.0f,-2.0f,2.0f,-3.0f},
        {2.0f,-1.0f,3.0f,-2.0f}, {1.0f,-1.0f,2.0f,-2.0f}
    };
    for (int m = 0; m < 4; m++) {
        csr = (saved & ~(UINT32_C(3) << 13)) | (modes[m] << 13);
        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        __asm__ volatile("movaps %1,%%xmm0\n\troundps $4,%%xmm0,%%xmm1\n\tmovaps %%xmm1,%0"
            : "=m"(r) : "m"(a) : "xmm0", "xmm1");
        for (int i = 0; i < 4; i++) TEST_ASSERT(r.f32[i] == expected[m][i], "roundps MXCSR mode %d lane %d", m, i);
    }

    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("movaps %0,%%xmm0\n\troundps $0,%%xmm0,%%xmm1" : : "m"(a) : "xmm0", "xmm1");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(csr & (1u << 5), "roundps unsuppressed precision flag");
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("movaps %0,%%xmm0\n\troundps $8,%%xmm0,%%xmm1" : : "m"(a) : "xmm0", "xmm1");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(!(csr & (1u << 5)), "roundps imm bit3 suppresses precision flag");

    a.u32[0] = 0x7f800001u; /* signaling NaN */
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("movaps %1,%%xmm0\n\troundps $8,%%xmm0,%%xmm1\n\tmovaps %%xmm1,%0"
        : "=m"(r) : "m"(a) : "xmm0", "xmm1");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
    TEST_ASSERT(isnan(r.f32[0]) && (r.u32[0] & 0x00400000u), "roundps quiets SNaN");
    TEST_ASSERT(csr & 1u, "roundps SNaN sets invalid flag");

    xmm_t pd = { .f64 = {1.9, -1.1} };
    const double expected_pd[4][2] = {
        {2.0, -1.0}, {1.0, -2.0}, {2.0, -1.0}, {1.0, -1.0}
    };
    for (int mode = 0; mode < 4; mode++) {
        csr = (saved & ~(UINT32_C(3) << 13)) | ((uint32_t)mode << 13);
        __asm__ volatile("ldmxcsr %0" : : "m"(csr));
        __asm__ volatile(
            "roundpd $4, %1, %%xmm0\n\t"
            "movapd %%xmm0, %0"
            : "=m"(r) : "m"(pd) : "xmm0"
        );
        for (int lane = 0; lane < 2; lane++)
            TEST_ASSERT(r.f64[lane] == expected_pd[mode][lane],
                        "roundpd MXCSR mode %d lane %d", mode, lane);
    }

    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("roundpd $0, %0, %%xmm0" : : "m"(pd) : "xmm0");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(csr & (1u << 5), "roundpd unsuppressed precision flag");
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("roundpd $8, %0, %%xmm0" : : "m"(pd) : "xmm0");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(!(csr & (1u << 5)), "roundpd imm bit3 suppresses precision flag");

    pd.u64[0] = UINT64_C(0x7ff0000000001234);
    pd.u64[1] = UINT64_C(0x8000000000000000);
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile(
        "roundpd $8, %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(pd) : "xmm0"
    );
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(r.u64[0] == UINT64_C(0x7ff8000000001234),
                "roundpd quiets SNaN with exact payload");
    TEST_ASSERT(r.u64[1] == UINT64_C(0x8000000000000000),
                "roundpd preserves negative zero bits");
    TEST_ASSERT(csr & 1u, "roundpd SNaN sets invalid despite imm bit3");
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
}

int main(void) {
    TEST_START("ROUNDPS/ROUNDPD instructions (SSE4.1)");
    test_roundps_nearest();
    test_roundps_floor();
    test_roundps_ceil();
    test_roundps_trunc();
    test_roundps_special();
    test_roundps_mem();
    test_roundpd_nearest();
    test_roundpd_floor();
    test_roundpd_ceil();
    test_roundpd_trunc();
    test_roundpd_special();
    test_roundpd_mem();
    test_round_mxcsr_and_exceptions();
    TEST_END();
}
