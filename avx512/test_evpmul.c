/*
 * Test VPMULLW/VPMULUDQ/VPMULHUW/VPMULHW/VPMADDWD with zmm registers (AVX-512BW/F).
 * Packed integer multiplication.
 *
 * Compile: gcc -o test_evpmul avx512/test_evpmul.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPMULLW/EVPMULUDQ/EVPMULHUW/EVPMULHW/EVPMADDWD (zmm)");

    zmm_t a, b, dst;

    /* VPMULLW: low 16 bits of signed word multiply */
    for (int i = 0; i < 32; i++) { a.i16[i] = (int16_t)(i + 1); b.i16[i] = (int16_t)(i + 2); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpmullw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++) {
        int16_t expected = (int16_t)((int)a.i16[i] * b.i16[i]);
        TEST_ASSERT(dst.i16[i] == expected,
            "VPMULLW lane %d: %d != %d", i, dst.i16[i], expected);
    }

    /* VPMULHW: high 16 bits of signed word multiply */
    for (int i = 0; i < 32; i++) { a.i16[i] = (int16_t)(i * 1000); b.i16[i] = (int16_t)((i + 1) * 500); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpmulhw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++) {
        int16_t expected = (int16_t)(((int)a.i16[i] * b.i16[i]) >> 16);
        TEST_ASSERT(dst.i16[i] == expected,
            "VPMULHW lane %d: %d != %d", i, dst.i16[i], expected);
    }

    /* VPMULHUW: high 16 bits of unsigned word multiply */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i * 1000 + 100); b.u16[i] = (uint16_t)((i + 1) * 500); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpmulhuw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++) {
        uint16_t expected = (uint16_t)(((unsigned)a.u16[i] * b.u16[i]) >> 16);
        TEST_ASSERT(dst.u16[i] == expected,
            "VPMULHUW lane %d: %u != %u", i, dst.u16[i], expected);
    }

    /* VPMULUDQ: unsigned dword multiply to qword (uses even dword lanes) */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i * 100000 + 1);
    for (int i = 0; i < 16; i++) b.u32[i] = (uint32_t)(i * 200000 + 3);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpmuludq %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (uint64_t)a.u32[i*2] * b.u32[i*2];
        TEST_ASSERT(dst.u64[i] == expected,
            "VPMULUDQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)expected);
    }

    /* VPMADDWD: multiply words and add adjacent pairs to dwords */
    for (int i = 0; i < 32; i++) { a.i16[i] = (int16_t)(i + 1); b.i16[i] = (int16_t)(32 - i); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpmaddwd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++) {
        int expected = (int)a.i16[i*2] * b.i16[i*2] + (int)a.i16[i*2+1] * b.i16[i*2+1];
        TEST_ASSERT(dst.i32[i] == expected,
            "VPMADDWD lane %d: %d != %d", i, dst.i32[i], expected);
    }

    /* Boundary: multiply by 0 */
    memset(&a, 0, sizeof(a));
    for (int i = 0; i < 32; i++) b.i16[i] = (int16_t)(i + 1);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpmullw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.i16[i] == 0, "VPMULLW zero-mul lane %d: %d", i, dst.i16[i]);

    TEST_END();
}
