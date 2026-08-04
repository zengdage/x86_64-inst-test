/*
 * Test VMAXPS/VMAXPD/VMINPS/VMINPD with zmm registers (AVX-512F).
 * Packed single/double precision floating-point max/min.
 *
 * Compile: gcc -o test_evmaxmin avx512/test_evmaxmin.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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
    TEST_START("EVMAXPS/EVMAXPD/EVMINPS/EVMINPD (VMAXPS/VMAXPD/VMINPS/VMINPD zmm)");

    zmm_t a, b, dst;

    /* VMAXPS */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)(i); b.f32[i] = (float)(15 - i); }
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmaxps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++) {
        float expected = a.f32[i] > b.f32[i] ? a.f32[i] : b.f32[i];
        TEST_ASSERT(dst.f32[i] == expected, "VMAXPS lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* VMINPS */
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vminps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++) {
        float expected = a.f32[i] < b.f32[i] ? a.f32[i] : b.f32[i];
        TEST_ASSERT(dst.f32[i] == expected, "VMINPS lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* VMAXPD */
    for (int i = 0; i < 8; i++) { a.f64[i] = (double)(i * 2); b.f64[i] = (double)(7 - i) * 2.0; }
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vmaxpd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++) {
        double expected = a.f64[i] > b.f64[i] ? a.f64[i] : b.f64[i];
        TEST_ASSERT(dst.f64[i] == expected, "VMAXPD lane %d: %f != %f", i, dst.f64[i], expected);
    }

    /* VMINPD */
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vminpd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++) {
        double expected = a.f64[i] < b.f64[i] ? a.f64[i] : b.f64[i];
        TEST_ASSERT(dst.f64[i] == expected, "VMINPD lane %d: %f != %f", i, dst.f64[i], expected);
    }

    /* VMAXPS with zeroing masking */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)(i + 1); b.f32[i] = (float)(16 - i); }
    uint64_t kmask = 0xAAAA;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmaxps %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        float expected = (kmask >> i) & 1 ? (a.f32[i] > b.f32[i] ? a.f32[i] : b.f32[i]) : 0.0f;
        TEST_ASSERT(dst.f32[i] == expected, "VMAXPS zero mask lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* NaN and signed-zero ties return the second source operand. */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)i; b.f32[i] = (float)(i + 1); }
    a.f32[0] = NAN;  b.f32[0] = 1.0f;
    a.f32[1] = 1.0f; b.f32[1] = NAN;
    a.f32[2] = 0.0f; b.f32[2] = -0.0f;
    a.f32[3] = -0.0f; b.f32[3] = 0.0f;
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmaxps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f && isnan(dst.f32[1]), "VMAXPS NaN source ordering");
    TEST_ASSERT(dst.f32[2] == 0.0f && signbit(dst.f32[2]), "VMAXPS +0,-0 returns -0");
    TEST_ASSERT(dst.f32[3] == 0.0f && !signbit(dst.f32[3]), "VMAXPS -0,+0 returns +0");

    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vminps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f && isnan(dst.f32[1]), "VMINPS NaN source ordering");
    TEST_ASSERT(dst.f32[2] == 0.0f && signbit(dst.f32[2]), "VMINPS +0,-0 returns -0");
    TEST_ASSERT(dst.f32[3] == 0.0f && !signbit(dst.f32[3]), "VMINPS -0,+0 returns +0");

    TEST_END();
}
