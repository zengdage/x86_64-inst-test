/*
 * Test VPSRAW/VPSRAD/VPSRAQ with zmm registers (AVX-512BW/F/DQ).
 * Packed integer arithmetic right shift (word/dword/qword).
 *
 * Compile: gcc -o test_evpsra avx512/test_evpsra.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPSRAW/EVPSRAD/EVPSRAQ (VPSRAW/VPSRAD/VPSRAQ zmm)");

    zmm_t a, dst;

    /* VPSRAW: arithmetic right shift by 3 (sign-extends) */
    for (int i = 0; i < 32; i++) a.i16[i] = (int16_t)(i < 16 ? -(i + 1) * 100 : (i + 1) * 100);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vpsraw $3, %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.i16[i] == (int16_t)(a.i16[i] >> 3),
            "VPSRAW lane %d: %d != %d", i, dst.i16[i], (int16_t)(a.i16[i] >> 3));

    /* VPSRAD: arithmetic right shift by 4 */
    for (int i = 0; i < 16; i++) a.i32[i] = (int)(i < 8 ? -(i + 1) * 10000 : (i + 1) * 10000);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrad $4, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i32[i] == a.i32[i] >> 4,
            "VPSRAD lane %d: %d != %d", i, dst.i32[i], a.i32[i] >> 4);

    /* VPSRAQ: arithmetic right shift by 8 (AVX-512F/DQ) */
    for (int i = 0; i < 8; i++) a.i64[i] = (long long)(i < 4 ? -(long long)(i + 1) * 0x100000000LL : (long long)(i + 1) * 0x100000000LL);
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vpsraq $8, %%zmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i64[i] == a.i64[i] >> 8,
            "VPSRAQ lane %d: %lld != %lld", i, (long long)dst.i64[i], (long long)(a.i64[i] >> 8));

    /* Boundary: negative >> 0 = same */
    for (int i = 0; i < 16; i++) a.i32[i] = -(i + 1);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrad $0, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i32[i] == a.i32[i], "VPSRAD shift-0 lane %d: %d", i, dst.i32[i]);

    /* Boundary: all-ones (all -1) >> 1 = -1 (sign fill) */
    memset(&a, 0xFF, sizeof(a));
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrad $1, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i32[i] == -1, "VPSRAD all-ones>>1 lane %d: %d", i, dst.i32[i]);

    /* VPSRAD with zeroing masking */
    for (int i = 0; i < 16; i++) a.i32[i] = -(i + 1) * 16;
    uint64_t kmask = 0x5555;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsrad $4, %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a), "m"(a), "r"(kmask) : "zmm0","zmm1","k1"
    );
    for (int i = 0; i < 16; i++) {
        int expected = (kmask >> i) & 1 ? a.i32[i] >> 4 : 0;
        TEST_ASSERT(dst.i32[i] == expected,
            "VPSRAD zero mask lane %d: %d != %d", i, dst.i32[i], expected);
    }

    TEST_END();
}
