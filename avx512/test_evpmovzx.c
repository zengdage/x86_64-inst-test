/*
 * Test VPMOVZXBD/VPMOVZXBQ/VPMOVZXDQ/VPMOVZXWD/VPMOVZXWQ with zmm (AVX-512F).
 * Packed zero-extend narrow to wide elements.
 *
 * Compile: gcc -o test_evpmovzx avx512/test_evpmovzx.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPMOVZXBD/EVPMOVZXBQ/EVPMOVZXDQ/EVPMOVZXWD/EVPMOVZXWQ (zmm)");

    zmm_t dst;

    /* VPMOVZXBD: 16 bytes -> 16 dwords (xmm src -> zmm dst) */
    xmm_t xsrc;
    for (int i = 0; i < 16; i++) xsrc.u8[i] = (uint8_t)(i * 16 + 0xFF - i * 15);
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovzxbd %%xmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == (uint32_t)xsrc.u8[i],
            "VPMOVZXBD lane %d: %u != %u", i, dst.u32[i], (uint32_t)xsrc.u8[i]);

    /* VPMOVZXBQ: 8 bytes -> 8 qwords */
    for (int i = 0; i < 8; i++) xsrc.u8[i] = (uint8_t)(i * 30 + 5);
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovzxbq %%xmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == (uint64_t)xsrc.u8[i],
            "VPMOVZXBQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)xsrc.u8[i]);

    /* VPMOVZXWD: 16 words -> 16 dwords (ymm src -> zmm dst) */
    ymm_t ysrc;
    for (int i = 0; i < 16; i++) ysrc.u16[i] = (uint16_t)(i * 4000 + 100);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vpmovzxwd %%ymm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(ysrc) : "ymm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == (uint32_t)ysrc.u16[i],
            "VPMOVZXWD lane %d: %u != %u", i, dst.u32[i], (uint32_t)ysrc.u16[i]);

    /* VPMOVZXWQ: 8 words -> 8 qwords (xmm src -> zmm dst) */
    for (int i = 0; i < 8; i++) xsrc.u16[i] = (uint16_t)(i * 8000 + 200);
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovzxwq %%xmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == (uint64_t)xsrc.u16[i],
            "VPMOVZXWQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)xsrc.u16[i]);

    /* VPMOVZXDQ: 8 dwords -> 8 qwords (ymm src -> zmm dst) */
    for (int i = 0; i < 8; i++) ysrc.u32[i] = (uint32_t)(i * 500000000U + 1);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vpmovzxdq %%ymm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(ysrc) : "ymm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == (uint64_t)ysrc.u32[i],
            "VPMOVZXDQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)ysrc.u32[i]);

    /* Boundary: all-ones zero extension (no sign fill) */
    memset(&xsrc, 0xFF, sizeof(xsrc));
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpmovzxbd %%xmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0xFF, "VPMOVZXBD all-ones lane %d: %u", i, dst.u32[i]);

    /* Zero and maximum boundaries for every source width. */
    for (int i = 0; i < 16; i++) xsrc.u8[i] = (i & 1) ? UINT8_MAX : 0;
    __asm__ volatile ("vmovdqu %1, %%xmm0\n\t" "vpmovzxbd %%xmm0, %%zmm1\n\t" "vmovdqu32 %%zmm1, %0"
                      : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == ((i & 1) ? UINT8_MAX : 0), "VPMOVZXBD extrema lane %d", i);
    __asm__ volatile ("vmovdqu %1, %%xmm0\n\t" "vpmovzxbq %%xmm0, %%zmm1\n\t" "vmovdqu64 %%zmm1, %0"
                      : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == ((i & 1) ? UINT8_MAX : 0), "VPMOVZXBQ extrema lane %d", i);

    for (int i = 0; i < 16; i++) ysrc.u16[i] = (i & 1) ? UINT16_MAX : 0;
    __asm__ volatile ("vmovdqu %1, %%ymm0\n\t" "vpmovzxwd %%ymm0, %%zmm1\n\t" "vmovdqu32 %%zmm1, %0"
                      : "=m"(dst) : "m"(ysrc) : "ymm0","zmm1");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == ((i & 1) ? UINT16_MAX : 0), "VPMOVZXWD extrema lane %d", i);
    for (int i = 0; i < 8; i++) xsrc.u16[i] = (i & 1) ? UINT16_MAX : 0;
    __asm__ volatile ("vmovdqu %1, %%xmm0\n\t" "vpmovzxwq %%xmm0, %%zmm1\n\t" "vmovdqu64 %%zmm1, %0"
                      : "=m"(dst) : "m"(xsrc) : "xmm0","zmm1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == ((i & 1) ? UINT16_MAX : 0), "VPMOVZXWQ extrema lane %d", i);

    for (int i = 0; i < 8; i++) ysrc.u32[i] = (i & 1) ? UINT32_MAX : 0;
    __asm__ volatile ("vmovdqu %1, %%ymm0\n\t" "vpmovzxdq %%ymm0, %%zmm1\n\t" "vmovdqu64 %%zmm1, %0"
                      : "=m"(dst) : "m"(ysrc) : "ymm0","zmm1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == ((i & 1) ? UINT32_MAX : 0), "VPMOVZXDQ extrema lane %d", i);

    uint32_t kmask = UINT32_C(0x8001);
    __asm__ volatile (
        "kmovd %2, %%k1\n\t" "vmovdqu %1, %%xmm0\n\t"
        "vpmovzxbd %%xmm0, %%zmm1%{%%k1%}%{z%}\n\t" "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(xsrc), "r"(kmask) : "xmm0","zmm1","k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (i == 0 || i == 15) ? xsrc.u8[i] : 0;
        TEST_ASSERT(dst.u32[i] == expected, "VPMOVZXBD endpoint zero mask lane %d", i);
    }

    TEST_END();
}
