/*
 * Test VPERMI2B/VPERMI2D/VPERMI2Q/VPERMI2W with zmm registers (AVX-512F/BW/VBMI).
 * Two-source permute with index vector.
 *
 * Compile: gcc -o test_evpermi2 avx512/test_evpermi2.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vbmi
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
static int check_avx512vbmi(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ecx >> 1) & 1;
}

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPERMI2B/EVPERMI2D/EVPERMI2Q/EVPERMI2W (zmm)");

    zmm_t src1, src2, idx, dst;

    /* VPERMI2D: index selects from src1 (idx[i]<16) or src2 (idx[i]>=16) */
    for (int i = 0; i < 16; i++) src1.u32[i] = (uint32_t)(i + 1);        /* 1..16 */
    for (int i = 0; i < 16; i++) src2.u32[i] = (uint32_t)(i + 100);      /* 100..115 */
    /* idx: even lanes from src2, odd lanes from src1 */
    for (int i = 0; i < 16; i++) idx.u32[i] = (i % 2 == 0) ? (uint32_t)(16 + i/2) : (uint32_t)(i/2);
    __asm__ volatile (
        "vmovdqu32 %2, %%zmm0\n\t"   /* src1 -> zmm0 (also dst) */
        "vmovdqu32 %3, %%zmm1\n\t"   /* idx  -> zmm1 */
        "vmovdqu32 %4, %%zmm2\n\t"   /* src2 -> zmm2 */
        "vpermi2d %%zmm2, %%zmm0, %%zmm1\n\t"  /* zmm1 = result */
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(src1), "m"(src1), "m"(idx), "m"(src2) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t id = idx.u32[i] & 0x1F;
        uint32_t expected = id < 16 ? src1.u32[id] : src2.u32[id - 16];
        TEST_ASSERT(dst.u32[i] == expected,
            "VPERMI2D lane %d: %u != %u", i, dst.u32[i], expected);
    }

    /* VPERMI2Q: 8 qword lanes */
    for (int i = 0; i < 8; i++) src1.u64[i] = (uint64_t)(i + 1) * 10;
    for (int i = 0; i < 8; i++) src2.u64[i] = (uint64_t)(i + 100) * 10;
    for (int i = 0; i < 8; i++) idx.u64[i] = (uint64_t)((i % 2 == 0) ? 8 + i/2 : i/2);
    __asm__ volatile (
        "vmovdqu64 %2, %%zmm0\n\t"
        "vmovdqu64 %3, %%zmm1\n\t"
        "vmovdqu64 %4, %%zmm2\n\t"
        "vpermi2q %%zmm2, %%zmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(src1), "m"(src1), "m"(idx), "m"(src2) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++) {
        uint64_t id = idx.u64[i] & 0xF;
        uint64_t expected = id < 8 ? src1.u64[id] : src2.u64[id - 8];
        TEST_ASSERT(dst.u64[i] == expected,
            "VPERMI2Q lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)expected);
    }

    /* VPERMI2W: 32 word lanes (AVX-512BW) */
    for (int i = 0; i < 32; i++) src1.u16[i] = (uint16_t)(i + 1);
    for (int i = 0; i < 32; i++) src2.u16[i] = (uint16_t)(i + 100);
    for (int i = 0; i < 32; i++) idx.u16[i] = (uint16_t)((i % 2 == 0) ? 32 + i/2 : i/2);
    __asm__ volatile (
        "vmovdqu16 %2, %%zmm0\n\t"
        "vmovdqu16 %3, %%zmm1\n\t"
        "vmovdqu16 %4, %%zmm2\n\t"
        "vpermi2w %%zmm2, %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(src1), "m"(src1), "m"(idx), "m"(src2) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++) {
        uint16_t id = idx.u16[i] & 0x3F;
        uint16_t expected = id < 32 ? src1.u16[id] : src2.u16[id - 32];
        TEST_ASSERT(dst.u16[i] == expected,
            "VPERMI2W lane %d: %u != %u", i, dst.u16[i], expected);
    }

    /* VPERMI2B: 64 byte lanes (AVX-512VBMI) */
    if (!check_avx512vbmi()) {
        printf("AVX-512VBMI not supported, skipping VPERMI2B test.\n");
    } else {
        for (int i = 0; i < 64; i++) src1.u8[i] = (uint8_t)(i + 1);
        for (int i = 0; i < 64; i++) src2.u8[i] = (uint8_t)(i + 100);
        for (int i = 0; i < 64; i++) idx.u8[i] = (uint8_t)((i % 2 == 0) ? 64 + i/2 : i/2);
        __asm__ volatile (
            "vmovdqu8 %2, %%zmm0\n\t"
            "vmovdqu8 %3, %%zmm1\n\t"
            "vmovdqu8 %4, %%zmm2\n\t"
            "vpermi2b %%zmm2, %%zmm0, %%zmm1\n\t"
            "vmovdqu8 %%zmm1, %0"
            : "=m"(dst) : "m"(src1), "m"(src1), "m"(idx), "m"(src2) : "zmm0","zmm1","zmm2"
        );
        for (int i = 0; i < 64; i++) {
            uint8_t id = idx.u8[i] & 0x7F;
            uint8_t expected = id < 64 ? src1.u8[id] : src2.u8[id - 64];
            TEST_ASSERT(dst.u8[i] == expected,
                "VPERMI2B lane %d: %u != %u", i, dst.u8[i], expected);
        }
    }

    TEST_END();
}
