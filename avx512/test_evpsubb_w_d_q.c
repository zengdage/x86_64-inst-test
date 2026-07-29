/*
 * Test VPSUBB/VPSUBW/VPSUBD/VPSUBQ with zmm registers (AVX-512BW/F).
 * Packed integer subtraction (byte/word/dword/qword).
 *
 * Compile: gcc -o test_evpsubb_w_d_q avx512/test_evpsubb_w_d_q.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPSUBB/EVPSUBW/EVPSUBD/EVPSUBQ (VPSUBB/VPSUBW/VPSUBD/VPSUBQ zmm)");

    zmm_t a, b, dst;

    /* VPSUBB */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i + 64); b.u8[i] = (uint8_t)i; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsubb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == (uint8_t)(a.u8[i] - b.u8[i]),
            "VPSUBB lane %d: %u != %u", i, dst.u8[i], (uint8_t)(a.u8[i]-b.u8[i]));

    /* VPSUBW */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i * 200 + 100); b.u16[i] = (uint16_t)(i * 100); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpsubw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == (uint16_t)(a.u16[i] - b.u16[i]),
            "VPSUBW lane %d: %u != %u", i, dst.u16[i], (uint16_t)(a.u16[i]-b.u16[i]));

    /* VPSUBD */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i * 1000 + 500); b.u32[i] = (uint32_t)(i * 300); }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpsubd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == a.u32[i] - b.u32[i],
            "VPSUBD lane %d: %u != %u", i, dst.u32[i], a.u32[i]-b.u32[i]);

    /* VPSUBQ */
    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)(i + 1) * 0x100000000ULL; b.u64[i] = (uint64_t)(i + 1); }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpsubq %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == a.u64[i] - b.u64[i],
            "VPSUBQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)(a.u64[i]-b.u64[i]));

    /* VPSUBD with zeroing masking */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i + 10); b.u32[i] = 5; }
    uint64_t kmask = 0x5555;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpsubd %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? a.u32[i] - 5 : 0;
        TEST_ASSERT(dst.u32[i] == expected,
            "VPSUBD zero mask lane %d: %u != %u", i, dst.u32[i], expected);
    }

    /* Boundary: subtract from self = 0 */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i * 7 + 3);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpsubd %%zmm0, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0, "VPSUBD self-sub lane %d: %u", i, dst.u32[i]);

    TEST_END();
}
