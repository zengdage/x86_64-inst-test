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

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512bw(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}
#else
#define check_avx512bw() 1
#endif

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

    /* Irregular data checks every 8-byte group against an independent oracle. */
    for (int i = 0; i < 64; i++) {
        a.u8[i] = (uint8_t)(i * 37 + 11);
        b.u8[i] = (uint8_t)(255 - i * 19);
    }
    uint64_t expected[8];
    for (int group = 0; group < 8; group++) {
        expected[group] = 0;
        for (int i = 0; i < 8; i++) {
            int diff = (int)a.u8[group * 8 + i] - (int)b.u8[group * 8 + i];
            expected[group] += (uint64_t)(diff < 0 ? -diff : diff);
        }
    }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vpsadbw %2, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm2"
    );
    for (int group = 0; group < 8; group++)
        TEST_ASSERT(dst.u64[group] == expected[group],
                    "VPSADBW zmm memory source group %d: %llu != %llu", group,
                    (unsigned long long)dst.u64[group],
                    (unsigned long long)expected[group]);

    /* VEX.256 computes four groups; VEX.128 computes two and clears YMM[255:128]. */
    ymm_t result256;
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vpsadbw %2, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(result256) : "m"(a), "m"(b) : "ymm0", "ymm2"
    );
    for (int group = 0; group < 4; group++)
        TEST_ASSERT(result256.u64[group] == expected[group],
                    "VPSADBW ymm group %d: %llu != %llu", group,
                    (unsigned long long)result256.u64[group],
                    (unsigned long long)expected[group]);

    ymm_t initial128, result128;
    memset(&initial128, 0xa5, sizeof(initial128));
    __asm__ volatile (
        "vmovdqu %1, %%ymm2\n\t"
        "vmovdqu %2, %%xmm0\n\t"
        "vpsadbw %3, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(result128) : "m"(initial128), "m"(a), "m"(b)
        : "xmm0", "ymm2"
    );
    TEST_ASSERT(result128.u64[0] == expected[0],
                "VPSADBW xmm low group: %llu != %llu",
                (unsigned long long)result128.u64[0],
                (unsigned long long)expected[0]);
    TEST_ASSERT(result128.u64[1] == expected[1],
                "VPSADBW xmm high group: %llu != %llu",
                (unsigned long long)result128.u64[1],
                (unsigned long long)expected[1]);
    TEST_ASSERT(result128.u64[2] == 0 && result128.u64[3] == 0,
                "VEX.128 VPSADBW clears upper YMM: %#llx %#llx",
                (unsigned long long)result128.u64[2],
                (unsigned long long)result128.u64[3]);

    TEST_END();
}
