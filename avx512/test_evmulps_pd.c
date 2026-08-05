/*
 * Test VMULPS/VMULPD with zmm registers (AVX-512F).
 * Packed single/double precision floating-point multiplication.
 *
 * Compile: gcc -o test_evmulps_pd avx512/test_evmulps_pd.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "../common.h"

typedef union {
    long long    i64[8];
    int          i32[16];
    short        i16[32];
    signed char  i8[64];
    unsigned long long u64[8];
    unsigned int       u32[16];
    unsigned short     u16[32];
    unsigned char      u8[64];
    float  f32[16];
    double f64[8];
} zmm_t __attribute__((aligned(64)));

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 16) & 1;
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVMULPS/EVMULPD (VMULPS/VMULPD zmm)");

    zmm_t a, b, dst;

    /* VMULPS basic */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)(i + 1); b.f32[i] = (float)(i + 2); }
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmulps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == a.f32[i] * b.f32[i], "VMULPS lane %d: %f != %f", i, dst.f32[i], a.f32[i]*b.f32[i]);

    /* VMULPD basic */
    for (int i = 0; i < 8; i++) { a.f64[i] = (double)(i + 1) * 1.5; b.f64[i] = (double)(i + 2) * 0.5; }
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vmulpd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.f64[i] == a.f64[i] * b.f64[i], "VMULPD lane %d: %f != %f", i, dst.f64[i], a.f64[i]*b.f64[i]);

    /* VMULPS with zeroing masking {k1}{z} */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)(i + 1); b.f32[i] = 2.0f; }
    uint64_t kmask = 0x5555; /* alternating */
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmulps %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        float expected = (kmask >> i) & 1 ? a.f32[i] * 2.0f : 0.0f;
        TEST_ASSERT(dst.f32[i] == expected, "VMULPS zero mask lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* Boundary: multiply by 1 */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)(i * 7 + 3); b.f32[i] = 1.0f; }
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmulps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == a.f32[i], "VMULPS mul-by-1 lane %d: %f", i, dst.f32[i]);

    /* Boundary: multiply by 0 */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)(i + 1); b.f32[i] = 0.0f; }
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmulps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == 0.0f, "VMULPS mul-by-0 lane %d: %f", i, dst.f32[i]);

    /* IEEE-754 and mask boundaries. */
    for (int i = 0; i < 16; i++) { a.f32[i] = 1.0f; b.f32[i] = 1.0f; }
    a.f32[0] = 0.0f; b.f32[0] = INFINITY;
    a.f32[1] = -0.0f; b.f32[1] = 2.0f;
    a.f32[2] = INFINITY; b.f32[2] = -2.0f;
    a.f32[3] = FLT_MAX; b.f32[3] = 2.0f;
    a.f32[4] = FLT_MIN; b.f32[4] = 0.5f;
    a.f32[5] = NAN; b.f32[5] = 1.0f;
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t" "vmovaps %2, %%zmm1\n\t"
        "vmulps %%zmm1, %%zmm0, %%zmm2\n\t" "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2"
    );
    TEST_ASSERT(IS_QNAN(dst.f32[0]), "VMULPS 0*Inf is QNaN");
    TEST_ASSERT(dst.f32[1] == 0.0f && signbit(dst.f32[1]), "VMULPS signed zero");
    TEST_ASSERT(isinf(dst.f32[2]) && signbit(dst.f32[2]), "VMULPS Inf*negative is -Inf");
    TEST_ASSERT(isinf(dst.f32[3]) && !signbit(dst.f32[3]), "VMULPS overflow");
    TEST_ASSERT(dst.f32[4] == FLT_MIN / 2.0f, "VMULPS subnormal");
    TEST_ASSERT(IS_QNAN(dst.f32[5]), "VMULPS QNaN propagation");

    a.f64[0] = 0.0; b.f64[0] = INFINITY;
    a.f64[1] = -0.0; b.f64[1] = 2.0;
    a.f64[2] = DBL_MAX; b.f64[2] = 2.0;
    a.f64[3] = DBL_MIN; b.f64[3] = 0.5;
    for (int i = 4; i < 8; i++) { a.f64[i] = 1.0; b.f64[i] = 1.0; }
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t" "vmovapd %2, %%zmm1\n\t"
        "vmulpd %%zmm1, %%zmm0, %%zmm2\n\t" "vmovapd %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2"
    );
    TEST_ASSERT(IS_QNAN(dst.f64[0]), "VMULPD 0*Inf is QNaN");
    TEST_ASSERT(dst.f64[1] == 0.0 && signbit(dst.f64[1]), "VMULPD signed zero");
    TEST_ASSERT(isinf(dst.f64[2]), "VMULPD overflow");
    TEST_ASSERT(dst.f64[3] == DBL_MIN / 2.0, "VMULPD subnormal");

    for (int i = 0; i < 16; i++) dst.f32[i] = 99.0f;
    zmm_t initial = dst;
    kmask = 0;
    __asm__ volatile (
        "kmovq %4, %%k1\n\t" "vmovaps %1, %%zmm0\n\t" "vmovaps %2, %%zmm1\n\t"
        "vmovaps %3, %%zmm2\n\t" "vmulps %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t" "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(initial), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1"
    );
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.f32[i] == 99.0f, "VMULPS k=0 merge lane %d", i);

    TEST_END();
}
