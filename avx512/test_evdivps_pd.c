/*
 * Test VDIVPS/VDIVPD with zmm registers (AVX-512F).
 * Packed single/double precision floating-point division.
 *
 * Compile: gcc -o test_evdivps_pd avx512/test_evdivps_pd.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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
    TEST_START("EVDIVPS/EVDIVPD (VDIVPS/VDIVPD zmm)");

    zmm_t a, b, dst;

    /* VDIVPS basic */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)((i + 1) * 4); b.f32[i] = (float)(i + 1); }
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vdivps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == 4.0f, "VDIVPS lane %d: %f != 4.0", i, dst.f32[i]);

    /* VDIVPD basic */
    for (int i = 0; i < 8; i++) { a.f64[i] = (double)((i + 1) * 8); b.f64[i] = (double)(i + 1); }
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vdivpd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.f64[i] == 8.0, "VDIVPD lane %d: %f != 8.0", i, dst.f64[i]);

    /* VDIVPS with zeroing masking {k1}{z} */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)((i + 1) * 2); b.f32[i] = 2.0f; }
    uint64_t kmask = 0xF0F0;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vdivps %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        float expected = (kmask >> i) & 1 ? (float)(i + 1) : 0.0f;
        TEST_ASSERT(dst.f32[i] == expected, "VDIVPS zero mask lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* Boundary: divide by self = 1 */
    for (int i = 0; i < 16; i++) a.f32[i] = (float)(i + 1) * 3.14f;
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vdivps %%zmm0, %%zmm0, %%zmm1\n\t"
        "vmovaps %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == 1.0f, "VDIVPS self-div lane %d: %f", i, dst.f32[i]);

    TEST_END();
}
