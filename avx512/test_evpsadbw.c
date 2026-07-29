/*
 * Test VPSADBW with zmm registers (AVX-512BW).
 * Packed sum of absolute differences of bytes, producing 64-bit results.
 *
 * Compile: gcc -o test_evpsadbw avx512/test_evpsadbw.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

static int check_avx512bw(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}

int main(void) {
    if (!check_avx512bw()) {
        printf("AVX-512BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPSADBW (VPSADBW zmm)");

    zmm_t a, b, dst;

    /*
     * VPSADBW: for each group of 8 bytes, compute sum of |a[i]-b[i]|
     * and store in the low 16 bits of each 64-bit lane (8 groups of 8 bytes).
     */

    /* All zeros: SAD = 0 */
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsadbw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == 0, "VPSADBW all-zero group %d: %llu", i, (unsigned long long)dst.u64[i]);

    /* a=255, b=0: SAD = 8*255 = 2040 per group */
    memset(&a, 0xFF, sizeof(a));
    memset(&b, 0, sizeof(b));
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsadbw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == 2040, "VPSADBW all-255 group %d: %llu", i, (unsigned long long)dst.u64[i]);

    /* Alternating: a[i]=i, b[i]=0 */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)i; b.u8[i] = 0; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsadbw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int g = 0; g < 8; g++) {
        uint64_t expected = 0;
        for (int j = 0; j < 8; j++) expected += a.u8[g*8+j];
        TEST_ASSERT(dst.u64[g] == expected,
            "VPSADBW sequential group %d: %llu != %llu", g,
            (unsigned long long)dst.u64[g], (unsigned long long)expected);
    }

    /* Verify upper bits of each qword result are zero */
    for (int i = 0; i < 8; i++)
        TEST_ASSERT((dst.u64[i] >> 16) == 0,
            "VPSADBW upper bits group %d: %llx", i, (unsigned long long)(dst.u64[i] >> 16));

    /* Symmetric: |a-b| = |b-a| */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i * 3); b.u8[i] = (uint8_t)(i * 2 + 10); }
    zmm_t dst2;
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsadbw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsadbw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst2) : "m"(b), "m"(a) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == dst2.u64[i],
            "VPSADBW symmetry group %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)dst2.u64[i]);

    TEST_END();
}
