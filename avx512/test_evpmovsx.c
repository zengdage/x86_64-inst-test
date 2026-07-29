/*
 * Test VPMOVSXBD/VPMOVSXBQ/VPMOVSXDQ/VPMOVSXWD/VPMOVSXWQ with zmm (AVX-512F).
 * Packed sign-extend narrow to wide elements.
 *
 * Compile: gcc -o test_evpmovsx avx512/test_evpmovsx.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPMOVSXBD/EVPMOVSXBQ/EVPMOVSXDQ/EVPMOVSXWD/EVPMOVSXWQ (zmm)");

    zmm_t dst;

    /* VPMOVSXBD: 16 bytes -> 16 dwords (xmm src -> zmm dst) */
    xmm_t xsrc;
    for (int i = 0; i < 16; i++) xsrc.i8[i] = (int8_t)(i - 8); /* -8..7 */
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovsxbd %%xmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i32[i] == (int)xsrc.i8[i],
            "VPMOVSXBD lane %d: %d != %d", i, dst.i32[i], (int)xsrc.i8[i]);

    /* VPMOVSXBQ: 8 bytes -> 8 qwords (xmm lower 8 bytes -> zmm) */
    for (int i = 0; i < 8; i++) xsrc.i8[i] = (int8_t)(i * 20 - 70);
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovsxbq %%xmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i64[i] == (long long)xsrc.i8[i],
            "VPMOVSXBQ lane %d: %lld != %lld", i, (long long)dst.i64[i], (long long)xsrc.i8[i]);

    /* VPMOVSXWD: 16 words -> 16 dwords (ymm src -> zmm dst) */
    ymm_t ysrc;
    for (int i = 0; i < 16; i++) ysrc.i16[i] = (int16_t)(i * 1000 - 8000);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vpmovsxwd %%ymm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(ysrc) : "ymm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i32[i] == (int)ysrc.i16[i],
            "VPMOVSXWD lane %d: %d != %d", i, dst.i32[i], (int)ysrc.i16[i]);

    /* VPMOVSXWQ: 8 words -> 8 qwords (xmm src -> zmm dst) */
    for (int i = 0; i < 8; i++) xsrc.i16[i] = (int16_t)(i * 5000 - 20000);
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovsxwq %%xmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i64[i] == (long long)xsrc.i16[i],
            "VPMOVSXWQ lane %d: %lld != %lld", i, (long long)dst.i64[i], (long long)xsrc.i16[i]);

    /* VPMOVSXDQ: 8 dwords -> 8 qwords (ymm src -> zmm dst) */
    for (int i = 0; i < 8; i++) ysrc.i32[i] = (int)(i * 100000 - 400000);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vpmovsxdq %%ymm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(ysrc) : "ymm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i64[i] == (long long)ysrc.i32[i],
            "VPMOVSXDQ lane %d: %lld != %lld", i, (long long)dst.i64[i], (long long)ysrc.i32[i]);

    /* Boundary: all-ones sign extension */
    memset(&xsrc, 0xFF, sizeof(xsrc));
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovsxbd %%xmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i32[i] == -1, "VPMOVSXBD all-ones lane %d: %d", i, dst.i32[i]);

    TEST_END();
}
