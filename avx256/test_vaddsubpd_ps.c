/*
 * Test VADDSUBPD/VADDSUBPS
 * 256-bit alternating add/sub on pairs: odd lanes add, even lanes subtract
 * Compile: gcc -o test_vaddsubpd_ps avx256/test_vaddsubpd_ps.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}
#else
#define check_avx() 1
#endif

static void test_vaddsubpd256(void) {
    TEST_START("VADDSUBPD (256-bit)");
    /* ymm: [d0,d1,d2,d3], result[i]: i even -> sub, i odd -> add */
    double a[4] = {10.0, 20.0, 30.0, 40.0};
    double b[4] = {1.0,  2.0,  3.0,  4.0};
    double r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vaddsubpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    /* even indices (0,2): sub; odd indices (1,3): add */
    TEST_ASSERT(r[0] == 9.0,  "vaddsubpd r[0] expected 9.0 got %f", r[0]);
    TEST_ASSERT(r[1] == 22.0, "vaddsubpd r[1] expected 22.0 got %f", r[1]);
    TEST_ASSERT(r[2] == 27.0, "vaddsubpd r[2] expected 27.0 got %f", r[2]);
    TEST_ASSERT(r[3] == 44.0, "vaddsubpd r[3] expected 44.0 got %f", r[3]);

    /* mem operand form */
    double r2[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vaddsubpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 9.0,  "vaddsubpd mem r[0] expected 9.0 got %f", r2[0]);
    TEST_ASSERT(r2[1] == 22.0, "vaddsubpd mem r[1] expected 22.0 got %f", r2[1]);
}

static void test_vaddsubps256(void) {
    TEST_START("VADDSUBPS (256-bit)");
    float a[8] = {10,20,30,40,50,60,70,80};
    float b[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vaddsubps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    /* even: sub, odd: add */
    TEST_ASSERT(r[0] == 9.0f,  "vaddsubps r[0] expected 9 got %f", r[0]);
    TEST_ASSERT(r[1] == 22.0f, "vaddsubps r[1] expected 22 got %f", r[1]);
    TEST_ASSERT(r[2] == 27.0f, "vaddsubps r[2] expected 27 got %f", r[2]);
    TEST_ASSERT(r[3] == 44.0f, "vaddsubps r[3] expected 44 got %f", r[3]);
    TEST_ASSERT(r[4] == 45.0f, "vaddsubps r[4] expected 45 got %f", r[4]);
    TEST_ASSERT(r[5] == 66.0f, "vaddsubps r[5] expected 66 got %f", r[5]);
    TEST_ASSERT(r[6] == 63.0f, "vaddsubps r[6] expected 63 got %f", r[6]);
    TEST_ASSERT(r[7] == 88.0f, "vaddsubps r[7] expected 88 got %f", r[7]);

    /* mem operand */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vaddsubps %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 9.0f, "vaddsubps mem r[0] expected 9 got %f", r2[0]);
}

static void test_vaddsub_special(void) {
    float a[8] = {INFINITY, -INFINITY, -0.0f, -0.0f, FLT_MAX, FLT_MIN, NAN, INFINITY};
    float b[8] = {INFINITY, -INFINITY, 0.0f, -0.0f, -FLT_MAX, FLT_MIN / 2.0f, 1.0f, -INFINITY};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t" "vmovups %2, %%ymm1\n\t"
        "vaddsubps %%ymm1, %%ymm0, %%ymm2\n\t" "vmovups %%ymm2, %0"
        : "=m"(r) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2"
    );
    TEST_ASSERT(IS_QNAN(r[0]), "vaddsubps even Inf-Inf is QNaN");
    TEST_ASSERT(isinf(r[1]) && signbit(r[1]), "vaddsubps odd -Inf+-Inf is -Inf");
    TEST_ASSERT(r[2] == 0.0f && signbit(r[2]), "vaddsubps even -0-+0 is -0");
    TEST_ASSERT(r[3] == 0.0f && signbit(r[3]), "vaddsubps odd -0+-0 is -0");
    TEST_ASSERT(isinf(r[4]) && !signbit(r[4]), "vaddsubps even overflow");
    TEST_ASSERT(r[5] == FLT_MIN * 1.5f, "vaddsubps odd subnormal add");
    TEST_ASSERT(IS_QNAN(r[6]) && IS_QNAN(r[7]),
                "vaddsubps QNaN and Inf cancellation produce QNaNs");

    double da[4] = {INFINITY, -0.0, DBL_MAX, DBL_MIN};
    double db[4] = {INFINITY, -0.0, -DBL_MAX, DBL_MIN / 2.0};
    double dr[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t" "vmovupd %2, %%ymm1\n\t"
        "vaddsubpd %%ymm1, %%ymm0, %%ymm2\n\t" "vmovupd %%ymm2, %0"
        : "=m"(dr) : "m"(da), "m"(db) : "ymm0", "ymm1", "ymm2"
    );
    TEST_ASSERT(IS_QNAN(dr[0]), "vaddsubpd even Inf-Inf is QNaN");
    TEST_ASSERT(dr[1] == 0.0 && signbit(dr[1]), "vaddsubpd odd -0+-0 is -0");
    TEST_ASSERT(isinf(dr[2]), "vaddsubpd even overflow");
    TEST_ASSERT(dr[3] == DBL_MIN * 1.5, "vaddsubpd odd subnormal add");
}

static void test_vaddsub_memory_all_lanes_flags_and_vex_clear(void) {
    float a[8] = {10,20,30,40,50,60,70,80};
    float b[8] = {1,2,3,4,5,6,7,8};
    float r[8];
    __asm__ volatile(
        "vmovups %1,%%ymm0\n\tvaddsubps %2,%%ymm0,%%ymm2\n\tvmovups %%ymm2,%0"
        : "=m"(r) : "m"(a), "m"(b) : "ymm0", "ymm2");
    for (int lane = 0; lane < 8; lane++) {
        float expected = (lane & 1) ? a[lane] + b[lane] : a[lane] - b[lane];
        TEST_ASSERT(r[lane] == expected, "vaddsubps memory source lane %d", lane);
    }

    double da[4] = {10,20,30,40}, db[4] = {1,2,3,4}, dr[4];
    __asm__ volatile(
        "vmovupd %1,%%ymm0\n\tvaddsubpd %2,%%ymm0,%%ymm2\n\tvmovupd %%ymm2,%0"
        : "=m"(dr) : "m"(da), "m"(db) : "ymm0", "ymm2");
    for (int lane = 0; lane < 4; lane++) {
        double expected = (lane & 1) ? da[lane] + db[lane] : da[lane] - db[lane];
        TEST_ASSERT(dr[lane] == expected, "vaddsubpd memory source lane %d", lane);
    }

    ymm_t initial, result;
    xmm_t xa = { .f32 = {10,20,30,40} }, xb = { .f32 = {1,2,3,4} };
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile(
        "vmovdqu %1,%%ymm2\n\tvmovups %2,%%xmm0\n\t"
        "vaddsubps %3,%%xmm0,%%xmm2\n\tvmovdqu %%ymm2,%0"
        : "=m"(result) : "m"(initial), "m"(xa), "m"(xb)
        : "xmm0", "ymm2");
    const float expected_xmm[4] = {9,22,27,44};
    for (int lane = 0; lane < 4; lane++)
        TEST_ASSERT(result.f32[lane] == expected_xmm[lane],
                    "vaddsubps VEX.128 lane %d", lane);
    TEST_ASSERT(result.u64[2] == 0 && result.u64[3] == 0,
                "vaddsubps VEX.128 clears upper YMM");

#if ENABLE_MXCSR_CHECK
    float exceptional_a[8] = {INFINITY, 1, FLT_MAX, 1, 0, 0, 0, 0};
    float exceptional_b[8] = {INFINITY, 1, -FLT_MAX, 1, 0, 0, 0, 0};
    uint32_t saved, csr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile(
        "vmovups %1,%%ymm0\n\tvaddsubps %2,%%ymm0,%%ymm2\n\tvmovups %%ymm2,%0"
        : "=m"(r) : "m"(exceptional_a), "m"(exceptional_b)
        : "ymm0", "ymm2");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(csr & 1u, "vaddsubps Inf-Inf sets invalid");
    TEST_ASSERT(csr & (1u << 3), "vaddsubps overflow sets overflow flag");
    TEST_ASSERT(csr & (1u << 5), "vaddsubps overflow sets precision flag");
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
#endif
}

#if ENABLE_MXCSR_CHECK
static void test_vaddsub_rounding_boundaries(void) {
    ymm_t fa = { .f32 = {1,1,1,1,1,1,1,1} };
    ymm_t fb = { .f32 = {
        0x1p-25f,0x1p-24f,0x1p-25f,0x1p-24f,
        0x1p-25f,0x1p-24f,0x1p-25f,0x1p-24f
    } };
    ymm_t nearest, upward, downward;
    uint32_t saved, csr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));

    csr = (saved & ~UINT32_C(0x603f));
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile (
        "vmovups %1, %%ymm0\n\tvaddsubps %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0"
        : "=m"(nearest) : "m"(fa), "m"(fb) : "ymm0", "ymm2");

    csr = (saved & ~UINT32_C(0x603f)) | UINT32_C(0x4000);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile (
        "vmovups %1, %%ymm0\n\tvaddsubps %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0"
        : "=m"(upward) : "m"(fa), "m"(fb) : "ymm0", "ymm2");

    csr = (saved & ~UINT32_C(0x603f)) | UINT32_C(0x2000);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile (
        "vmovups %1, %%ymm0\n\tvaddsubps %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0"
        : "=m"(downward) : "m"(fa), "m"(fb) : "ymm0", "ymm2");

    for (int lane = 0; lane < 8; lane++) {
        uint32_t up_expected = (lane & 1) ? UINT32_C(0x3f800001) : UINT32_C(0x3f800000);
        uint32_t down_expected = (lane & 1) ? UINT32_C(0x3f800000) : UINT32_C(0x3f7fffff);
        TEST_ASSERT(nearest.u32[lane] == UINT32_C(0x3f800000),
                    "vaddsubps nearest-even half-ULP lane %d", lane);
        TEST_ASSERT(upward.u32[lane] == up_expected,
                    "vaddsubps upward half-ULP add/sub lane %d", lane);
        TEST_ASSERT(downward.u32[lane] == down_expected,
                    "vaddsubps downward half-ULP add/sub lane %d", lane);
    }

    ymm_t da = { .f64 = {1,1,1,1} };
    ymm_t db = { .f64 = {0x1p-54,0x1p-53,0x1p-54,0x1p-53} };
    csr = (saved & ~UINT32_C(0x603f));
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile (
        "vmovupd %1, %%ymm0\n\tvaddsubpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0"
        : "=m"(nearest) : "m"(da), "m"(db) : "ymm0", "ymm2");
    csr = (saved & ~UINT32_C(0x603f)) | UINT32_C(0x4000);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile (
        "vmovupd %1, %%ymm0\n\tvaddsubpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0"
        : "=m"(upward) : "m"(da), "m"(db) : "ymm0", "ymm2");
    csr = (saved & ~UINT32_C(0x603f)) | UINT32_C(0x2000);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile (
        "vmovupd %1, %%ymm0\n\tvaddsubpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0"
        : "=m"(downward) : "m"(da), "m"(db) : "ymm0", "ymm2");
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));

    for (int lane = 0; lane < 4; lane++) {
        uint64_t up_expected = (lane & 1) ? UINT64_C(0x3ff0000000000001) :
                                           UINT64_C(0x3ff0000000000000);
        uint64_t down_expected = (lane & 1) ? UINT64_C(0x3ff0000000000000) :
                                             UINT64_C(0x3fefffffffffffff);
        TEST_ASSERT(nearest.u64[lane] == UINT64_C(0x3ff0000000000000),
                    "vaddsubpd nearest-even half-ULP lane %d", lane);
        TEST_ASSERT(upward.u64[lane] == up_expected,
                    "vaddsubpd upward half-ULP add/sub lane %d", lane);
        TEST_ASSERT(downward.u64[lane] == down_expected,
                    "vaddsubpd downward half-ULP add/sub lane %d", lane);
    }
}
#endif

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vaddsubpd256();
    test_vaddsubps256();
    test_vaddsub_special();
    test_vaddsub_memory_all_lanes_flags_and_vex_clear();
#if ENABLE_MXCSR_CHECK
    test_vaddsub_rounding_boundaries();
#endif
    TEST_END();
}
