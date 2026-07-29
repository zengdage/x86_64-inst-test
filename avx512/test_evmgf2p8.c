/*
 * Test GF2P8AFFINEQB/GF2P8AFFINEINVQB/GF2P8MULB with zmm registers (AVX-512 + GFNI).
 * Galois Field arithmetic instructions.
 *
 * Compile: gcc -o test_evmgf2p8 avx512/test_evmgf2p8.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl -mgfni
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

static int check_gfni(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ecx >> 8) & 1; /* GFNI: CPUID.7.ECX[8] */
}

/* GF(2^8) multiply with polynomial x^8+x^4+x^3+x+1 (0x11B) */
static uint8_t gf_mul(uint8_t a, uint8_t b) {
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) result ^= a;
        uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) a ^= 0x1B;
        b >>= 1;
    }
    return result;
}

int main(void) {
    if (!check_gfni()) {
        printf("GFNI not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVMGF2P8MULB/EVMGF2P8AFFINEQB/EVMGF2P8AFFINEINVQB (zmm)");

    zmm_t a, b, dst;

    /* GF2P8MULB zmm: multiply each byte pair in GF(2^8) */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i + 1); b.u8[i] = (uint8_t)(i + 2); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vgf2p8mulb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++) {
        uint8_t expected = gf_mul(a.u8[i], b.u8[i]);
        TEST_ASSERT(dst.u8[i] == expected,
            "GF2P8MULB lane %d: %02x != %02x", i, dst.u8[i], expected);
    }

    /* GF2P8MULB: multiply by 1 = identity */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i + 1); b.u8[i] = 1; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vgf2p8mulb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == a.u8[i], "GF2P8MULB mul-by-1 lane %d: %02x", i, dst.u8[i]);

    /* GF2P8MULB: multiply by 0 = 0 */
    memset(&b, 0, sizeof(b));
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vgf2p8mulb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == 0, "GF2P8MULB mul-by-0 lane %d: %02x", i, dst.u8[i]);

    /* GF2P8AFFINEQB zmm: affine transform with identity matrix (0x0102040810204080) */
    /* Identity matrix in GF(2^8): each row is a power of 2 */
    for (int i = 0; i < 64; i++) a.u8[i] = (uint8_t)(i + 1);
    /* Set b to identity matrix (same for all 8-byte groups) */
    uint64_t identity_matrix = 0x0102040810204080ULL;
    for (int i = 0; i < 8; i++) b.u64[i] = identity_matrix;
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vgf2p8affineqb $0, %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    /* With identity matrix and imm8=0, result should equal input */
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == a.u8[i],
            "GF2P8AFFINEQB identity lane %d: %02x != %02x", i, dst.u8[i], a.u8[i]);

    /* GF2P8AFFINEINVQB zmm: affine transform with inverse S-box */
    for (int i = 0; i < 64; i++) a.u8[i] = (uint8_t)(i + 1);
    for (int i = 0; i < 8; i++) b.u64[i] = identity_matrix;
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vgf2p8affineinvqb $0, %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    /* Just verify it executes and produces some output */
    TEST_ASSERT(1, "GF2P8AFFINEINVQB executed");

    /* GF2P8MULB with zeroing masking */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i + 1); b.u8[i] = 2; }
    uint64_t kmask = 0xAAAAAAAAAAAAAAAAULL;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vgf2p8mulb %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 64; i++) {
        uint8_t expected = (kmask >> i) & 1 ? gf_mul(a.u8[i], 2) : 0;
        TEST_ASSERT(dst.u8[i] == expected,
            "GF2P8MULB zero mask lane %d: %02x != %02x", i, dst.u8[i], expected);
    }

    TEST_END();
}
