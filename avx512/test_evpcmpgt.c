/*
 * Test VPCMPGTB/VPCMPGTW/VPCMPGTD/VPCMPGTQ with zmm -> k register (AVX-512BW/F).
 * Packed compare greater-than producing mask results.
 *
 * Compile: gcc -o test_evpcmpgt avx512/test_evpcmpgt.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPCMPGTB/EVPCMPGTW/EVPCMPGTD/EVPCMPGTQ (zmm -> k)");

    zmm_t a, b;
    uint64_t kmask;

    /* VPCMPGTD: a > b signed */
    for (int i = 0; i < 16; i++) { a.i32[i] = i * 2; b.i32[i] = i; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpcmpgtd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    /* lane 0: 0 > 0 = false; lanes 1-15: 2i > i = true */
    TEST_ASSERT((kmask & 0xFFFF) == 0xFFFE, "VPCMPGTD basic: mask=%04llx", (unsigned long long)kmask);

    /* VPCMPGTD: none greater */
    for (int i = 0; i < 16; i++) { a.i32[i] = i; b.i32[i] = i * 2; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpcmpgtd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT((kmask & 0xFFFF) == 0, "VPCMPGTD none-greater: mask=%04llx", (unsigned long long)kmask);

    /* VPCMPGTD: signed negative */
    for (int i = 0; i < 16; i++) { a.i32[i] = 0; b.i32[i] = -1; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpcmpgtd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT((kmask & 0xFFFF) == 0xFFFF, "VPCMPGTD 0>-1: mask=%04llx", (unsigned long long)kmask);

    /* VPCMPGTQ: 8 qword lanes */
    for (int i = 0; i < 8; i++) { a.i64[i] = (long long)(i * 10 + 5); b.i64[i] = (long long)(i * 10); }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpcmpgtq %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT((kmask & 0xFF) == 0xFF, "VPCMPGTQ all-greater: mask=%02llx", (unsigned long long)kmask);

    /* VPCMPGTB: 64 byte lanes, alternating */
    for (int i = 0; i < 64; i++) { a.i8[i] = (int8_t)(i % 2 == 0 ? 10 : -10); b.i8[i] = 0; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpcmpgtb %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0x5555555555555555ULL, "VPCMPGTB alternating: mask=%016llx", (unsigned long long)kmask);

    /* VPCMPGTW: 32 word lanes */
    for (int i = 0; i < 32; i++) { a.i16[i] = (int16_t)(i < 16 ? 100 : -100); b.i16[i] = 0; }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpcmpgtw %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT((kmask & 0xFFFFFFFF) == 0x0000FFFF, "VPCMPGTW lower16-greater: mask=%08llx", (unsigned long long)kmask);

    TEST_END();
}
