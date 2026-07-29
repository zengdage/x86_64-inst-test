/*
 * Test VPBLENDMB/VPBLENDMD/VPBLENDMQ/VPBLENDMW with zmm registers (AVX-512BW/F).
 * Packed blend using merge masking: dst[i] = k[i] ? src1[i] : src2[i].
 *
 * Compile: gcc -o test_evpblend avx512/test_evpblend.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512F/BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPBLENDMB/EVPBLENDMD/EVPBLENDMQ/EVPBLENDMW (zmm merge masking)");

    zmm_t a, b, dst;
    uint64_t kmask;

    /* VPBLENDMD: blend dwords, k1=0xAAAA -> odd lanes from a, even from b */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i + 100); b.u32[i] = (uint32_t)(i + 200); }
    kmask = 0xAAAA;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpblendmd %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? a.u32[i] : b.u32[i];
        TEST_ASSERT(dst.u32[i] == expected,
            "VPBLENDMD lane %d: %u != %u", i, dst.u32[i], expected);
    }

    /* VPBLENDMQ: blend qwords */
    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)(i + 10) * 100; b.u64[i] = (uint64_t)(i + 20) * 100; }
    kmask = 0xAA; /* alternating */
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpblendmq %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (kmask >> i) & 1 ? a.u64[i] : b.u64[i];
        TEST_ASSERT(dst.u64[i] == expected,
            "VPBLENDMQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)expected);
    }

    /* VPBLENDMW: blend words */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i + 1); b.u16[i] = (uint16_t)(i + 100); }
    kmask = 0xFFFF0000ULL; /* upper 16 lanes from a */
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpblendmw %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 32; i++) {
        uint16_t expected = (kmask >> i) & 1 ? a.u16[i] : b.u16[i];
        TEST_ASSERT(dst.u16[i] == expected,
            "VPBLENDMW lane %d: %u != %u", i, dst.u16[i], expected);
    }

    /* VPBLENDMB: blend bytes */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i + 1); b.u8[i] = (uint8_t)(i + 128); }
    kmask = 0xAAAAAAAAAAAAAAAAULL;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpblendmb %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 64; i++) {
        uint8_t expected = (kmask >> i) & 1 ? a.u8[i] : b.u8[i];
        TEST_ASSERT(dst.u8[i] == expected,
            "VPBLENDMB lane %d: %u != %u", i, dst.u8[i], expected);
    }

    /* All-ones mask: all from a */
    kmask = 0xFFFF;
    for (int i = 0; i < 16; i++) { a.u32[i] = 0xAAAAAAAAU; b.u32[i] = 0x55555555U; }
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpblendmd %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0xAAAAAAAAU, "VPBLENDMD all-a lane %d: %08x", i, dst.u32[i]);

    TEST_END();
}
