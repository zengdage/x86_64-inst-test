/*
 * Test VADDPS/VADDPD with zmm registers (AVX-512F).
 * Packed single/double precision floating-point addition.
 *
 * Compile: gcc -o test_evaddps_pd avx512/test_evaddps_pd.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 16) & 1;
}

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVADDPS/EVADDPD (VADDPS/VADDPD zmm)");

    zmm_t a, b, dst;

    /* VADDPS: basic */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)i; b.f32[i] = (float)(i + 1); }
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vaddps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == a.f32[i] + b.f32[i], "VADDPS lane %d: %f != %f", i, dst.f32[i], a.f32[i]+b.f32[i]);

    /* VADDPD: basic */
    for (int i = 0; i < 8; i++) { a.f64[i] = (double)i * 1.5; b.f64[i] = (double)(i + 2) * 0.5; }
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vaddpd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.f64[i] == a.f64[i] + b.f64[i], "VADDPD lane %d: %f != %f", i, dst.f64[i], a.f64[i]+b.f64[i]);

    /* VADDPS with merge masking {k1} */
    for (int i = 0; i < 16; i++) { a.f32[i] = 1.0f; b.f32[i] = 2.0f; dst.f32[i] = 99.0f; }
    uint64_t kmask = 0xFF00; /* upper 8 lanes */
    zmm_t orig; memcpy(&orig, &dst, 64);
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovaps %4, %%zmm0\n\t"
        "vmovaps %5, %%zmm1\n\t"
        "vmovaps %6, %%zmm2\n\t"
        "vaddps %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst)
        : "m"(orig), "m"(orig), "r"(kmask), "m"(a), "m"(b), "m"(orig)
        : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        float expected = (kmask >> i) & 1 ? 3.0f : 99.0f;
        TEST_ASSERT(dst.f32[i] == expected, "VADDPS merge mask lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* VADDPS with zeroing masking {k1}{z} */
    for (int i = 0; i < 16; i++) { a.f32[i] = 1.0f; b.f32[i] = 2.0f; }
    kmask = 0x00FF; /* lower 8 lanes */
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vaddps %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        float expected = (kmask >> i) & 1 ? 3.0f : 0.0f;
        TEST_ASSERT(dst.f32[i] == expected, "VADDPS zero mask lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* Boundary: zeros */
    memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b));
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vaddps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == 0.0f, "VADDPS zero boundary lane %d", i);

    TEST_END();
}
