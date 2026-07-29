/*
 * Test VPSRLW/VPSRLD/VPSRLQ with zmm registers (AVX-512BW/F).
 * Packed integer logical right shift (word/dword/qword).
 *
 * Compile: gcc -o test_evpsrl avx512/test_evpsrl.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPSRLW/EVPSRLD/EVPSRLQ (VPSRLW/VPSRLD/VPSRLQ zmm)");

    zmm_t a, dst;

    /* VPSRLW: shift right by 3 */
    for (int i = 0; i < 32; i++) a.u16[i] = (uint16_t)(i * 8 + 0xFF);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vpsrlw $3, %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == (uint16_t)(a.u16[i] >> 3),
            "VPSRLW lane %d: %u != %u", i, dst.u16[i], (uint16_t)(a.u16[i] >> 3));

    /* VPSRLD: shift right by 4 */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i * 0x10000 + 0xFFFF);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrld $4, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == a.u32[i] >> 4,
            "VPSRLD lane %d: %u != %u", i, dst.u32[i], a.u32[i] >> 4);

    /* VPSRLQ: shift right by 8 */
    for (int i = 0; i < 8; i++) a.u64[i] = (uint64_t)(i + 1) * 0x100000000ULL;
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vpsrlq $8, %%zmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == a.u64[i] >> 8,
            "VPSRLQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)(a.u64[i] >> 8));

    /* Boundary: shift by 0 */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i * 7 + 1);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrld $0, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == a.u32[i], "VPSRLD shift-0 lane %d: %u", i, dst.u32[i]);

    /* Boundary: all-ones >> 1 = 0x7FFF... */
    memset(&a, 0xFF, sizeof(a));
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrld $1, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0x7FFFFFFFU, "VPSRLD all-ones>>1 lane %d: %08x", i, dst.u32[i]);

    /* VPSRLD with zeroing masking */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i + 1) * 16;
    uint64_t kmask = 0xAAAA;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrld $4, %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a), "m"(a), "r"(kmask) : "zmm0","zmm1","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? a.u32[i] >> 4 : 0;
        TEST_ASSERT(dst.u32[i] == expected,
            "VPSRLD zero mask lane %d: %u != %u", i, dst.u32[i], expected);
    }

    TEST_END();
}
