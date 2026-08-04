/*
 * Test VPERMD/VPERMQ/VPERMB/VPERMW with zmm registers (AVX-512F/BW/VBMI).
 * Packed permute dword/qword/byte/word.
 *
 * Compile: gcc -o test_evperm avx512/test_evperm.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vbmi
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
static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 16) & 1;
}
#else
#define check_avx512() 1
#endif
#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512vbmi(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ecx >> 1) & 1;
}
#else
#define check_avx512vbmi() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPERMD/EVPERMQ/EVPERMB/EVPERMW (VPERMD/VPERMQ/VPERMB/VPERMW zmm)");

    zmm_t src, idx, dst;

    /* VPERMD: permute 16 dwords using index vector */
    for (int i = 0; i < 16; i++) src.u32[i] = (uint32_t)(i + 1) * 10;
    /* reverse order */
    for (int i = 0; i < 16; i++) idx.u32[i] = (uint32_t)(15 - i);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpermd %%zmm0, %%zmm1, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == src.u32[15 - i],
            "VPERMD lane %d: %u != %u", i, dst.u32[i], src.u32[15-i]);

    /* VPERMQ: permute 8 qwords using index vector */
    for (int i = 0; i < 8; i++) src.u64[i] = (uint64_t)(i + 1) * 100;
    for (int i = 0; i < 8; i++) idx.u64[i] = (uint64_t)(7 - i);
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpermq %%zmm0, %%zmm1, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == src.u64[7 - i],
            "VPERMQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)src.u64[7-i]);

    /* VPERMW: permute 32 words (AVX-512BW) */
    for (int i = 0; i < 32; i++) src.u16[i] = (uint16_t)(i + 1);
    for (int i = 0; i < 32; i++) idx.u16[i] = (uint16_t)(31 - i);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpermw %%zmm0, %%zmm1, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == src.u16[31 - i],
            "VPERMW lane %d: %u != %u", i, dst.u16[i], src.u16[31-i]);

    /* VPERMB: permute 64 bytes (AVX-512VBMI) */
    if (!check_avx512vbmi()) {
        printf("AVX-512VBMI not supported, skipping VPERMB test.\n");
    } else {
        for (int i = 0; i < 64; i++) src.u8[i] = (uint8_t)(i + 1);
        for (int i = 0; i < 64; i++) idx.u8[i] = (uint8_t)(63 - i);
        __asm__ volatile (
            "vmovdqu8 %1, %%zmm0\n\t"
            "vmovdqu8 %2, %%zmm1\n\t"
            "vpermb %%zmm0, %%zmm1, %%zmm2\n\t"
            "vmovdqu8 %%zmm2, %0"
            : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
        );
        for (int i = 0; i < 64; i++)
            TEST_ASSERT(dst.u8[i] == src.u8[63 - i],
                "VPERMB lane %d: %u != %u", i, dst.u8[i], src.u8[63-i]);
    }

    /* VPERMD with zeroing masking */
    for (int i = 0; i < 16; i++) { src.u32[i] = (uint32_t)(i + 1); idx.u32[i] = (uint32_t)(15 - i); }
    uint64_t kmask = 0x00FF;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpermd %%zmm0, %%zmm1, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? src.u32[15 - i] : 0;
        TEST_ASSERT(dst.u32[i] == expected,
            "VPERMD zero mask lane %d: %u != %u", i, dst.u32[i], expected);
    }

    /* Index high bits are ignored according to the element width. */
    const uint32_t didx[16] = {0,15,16,31,32,47,UINT32_MAX,0x8000000fU,1,14,17,30,33,63,64,127};
    for (int i = 0; i < 16; i++) { src.u32[i] = 0x1000u + (uint32_t)i; idx.u32[i] = didx[i]; }
    __asm__ volatile ("vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvpermd %%zmm0,%%zmm1,%%zmm2\n\tvmovdqu32 %%zmm2,%0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == src.u32[idx.u32[i] & 15], "VPERMD wrapped index lane %d", i);

    const uint64_t qidx[8] = {0,7,8,15,16,UINT64_MAX,0x8000000000000007ULL,31};
    for (int i = 0; i < 8; i++) { src.u64[i] = 0x2000u + (uint64_t)i; idx.u64[i] = qidx[i]; }
    __asm__ volatile ("vmovdqu64 %1,%%zmm0\n\tvmovdqu64 %2,%%zmm1\n\tvpermq %%zmm0,%%zmm1,%%zmm2\n\tvmovdqu64 %%zmm2,%0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == src.u64[idx.u64[i] & 7], "VPERMQ wrapped index lane %d", i);

    for (int i = 0; i < 32; i++) { src.u16[i] = (uint16_t)(0x3000 + i); idx.u16[i] = (uint16_t)(i + 32); }
    idx.u16[0] = UINT16_MAX;
    __asm__ volatile ("vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpermw %%zmm0,%%zmm1,%%zmm2\n\tvmovdqu16 %%zmm2,%0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 32; i++) TEST_ASSERT(dst.u16[i] == src.u16[idx.u16[i] & 31], "VPERMW wrapped index lane %d", i);

    if (check_avx512vbmi()) {
        for (int i = 0; i < 64; i++) { src.u8[i] = (uint8_t)i; idx.u8[i] = (uint8_t)(i + 64); }
        idx.u8[0] = UINT8_MAX;
        __asm__ volatile ("vmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvpermb %%zmm0,%%zmm1,%%zmm2\n\tvmovdqu8 %%zmm2,%0"
            : "=m"(dst) : "m"(src), "m"(idx) : "zmm0", "zmm1", "zmm2");
        for (int i = 0; i < 64; i++) TEST_ASSERT(dst.u8[i] == src.u8[idx.u8[i] & 63], "VPERMB wrapped index lane %d", i);
    }

    for (int i = 0; i < 16; i++) { src.u32[i] = 100 + (uint32_t)i; idx.u32[i] = (uint32_t)(15 - i); dst.u32[i] = 0xdeadbeefu; }
    zmm_t initial = dst;
    const uint64_t masks[] = {0, 0xffff, 0x8001};
    for (unsigned m = 0; m < sizeof(masks)/sizeof(masks[0]); m++) {
        kmask = masks[m];
        __asm__ volatile ("kmovq %4,%%k1\n\tvmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvmovdqu32 %3,%%zmm2\n\tvpermd %%zmm0,%%zmm1,%%zmm2%{%%k1%}\n\tvmovdqu32 %%zmm2,%0"
            : "=m"(dst) : "m"(src), "m"(idx), "m"(initial), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
        for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == (((kmask >> i)&1) ? src.u32[idx.u32[i]&15] : initial.u32[i]), "VPERMD merge mask 0x%04llx lane %d", (unsigned long long)kmask, i);
        __asm__ volatile ("kmovq %3,%%k1\n\tvmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvpermd %%zmm0,%%zmm1,%%zmm2%{%%k1%}%{z%}\n\tvmovdqu32 %%zmm2,%0"
            : "=m"(dst) : "m"(src), "m"(idx), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
        for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == (((kmask >> i)&1) ? src.u32[idx.u32[i]&15] : 0), "VPERMD zero mask 0x%04llx lane %d", (unsigned long long)kmask, i);
    }

    TEST_END();
}
