/*
 * Test VPERMB/VPERMD/VPERMQ/VPERMW as EVPERMUT* variants (same instructions).
 * These map to the same VPERMB/VPERMD/VPERMQ/VPERMW zmm instructions.
 *
 * Compile: gcc -o test_evpermut avx512/test_evpermut.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vbmi
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
    TEST_START("EVPERMUTB/EVPERMUTD/EVPERMUTQ/EVPERMUTW (VPERMB/VPERMD/VPERMQ/VPERMW zmm)");

    zmm_t src, idx, dst;

    /* EVPERMUTD -> VPERMD: identity permutation */
    for (int i = 0; i < 16; i++) { src.u32[i] = (uint32_t)(i + 1) * 7; idx.u32[i] = (uint32_t)i; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpermd %%zmm0, %%zmm1, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == src.u32[i], "EVPERMUTD identity lane %d: %u", i, dst.u32[i]);

    /* EVPERMUTD -> VPERMD: broadcast lane 0 */
    for (int i = 0; i < 16; i++) idx.u32[i] = 0;
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpermd %%zmm0, %%zmm1, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == src.u32[0], "EVPERMUTD broadcast lane %d: %u", i, dst.u32[i]);

    /* EVPERMUTQ -> VPERMQ: reverse */
    for (int i = 0; i < 8; i++) { src.u64[i] = (uint64_t)(i + 1) * 100; idx.u64[i] = (uint64_t)(7 - i); }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpermq %%zmm0, %%zmm1, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == src.u64[7 - i],
            "EVPERMUTQ reverse lane %d: %llu", i, (unsigned long long)dst.u64[i]);

    /* EVPERMUTW -> VPERMW: rotate by 1 */
    for (int i = 0; i < 32; i++) { src.u16[i] = (uint16_t)(i + 1); idx.u16[i] = (uint16_t)((i + 1) % 32); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpermw %%zmm0, %%zmm1, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == src.u16[(i + 1) % 32],
            "EVPERMUTW rotate lane %d: %u", i, dst.u16[i]);

    /* EVPERMUTB -> VPERMB (AVX-512VBMI) */
    if (!check_avx512vbmi()) {
        printf("AVX-512VBMI not supported, skipping EVPERMUTB test.\n");
    } else {
        for (int i = 0; i < 64; i++) { src.u8[i] = (uint8_t)(i + 1); idx.u8[i] = (uint8_t)((i + 3) % 64); }
        __asm__ volatile (
            "vmovdqu8 %1, %%zmm0\n\t"
            "vmovdqu8 %2, %%zmm1\n\t"
            "vpermb %%zmm0, %%zmm1, %%zmm2\n\t"
            "vmovdqu8 %%zmm2, %0"
            : "=m"(dst) : "m"(src), "m"(idx) : "zmm0","zmm1","zmm2"
        );
        for (int i = 0; i < 64; i++)
            TEST_ASSERT(dst.u8[i] == src.u8[(i + 3) % 64],
                "EVPERMUTB rotate lane %d: %u", i, dst.u8[i]);
    }

    /* Complement the reverse/identity cases with wrapped indices and mask extremes. */
    for (int i = 0; i < 16; i++) { src.u32[i] = 0x5000u + (uint32_t)i; idx.u32[i] = UINT32_MAX - (uint32_t)i; }
    __asm__ volatile ("vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvpermd %%zmm0,%%zmm1,%%zmm2\n\tvmovdqu32 %%zmm2,%0"
        : "=m"(dst) : "m"(src), "m"(idx) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == src.u32[idx.u32[i] & 15], "EVPERMUTD high-index wrap lane %d", i);

    for (int i = 0; i < 8; i++) { src.u64[i] = 0x6000u + (uint64_t)i; idx.u64[i] = UINT64_MAX - (uint64_t)i; dst.u64[i] = 0xdeadbeefdeadbeefULL; }
    zmm_t initial = dst;
    uint64_t kmask = 0;
    __asm__ volatile ("kmovq %4,%%k1\n\tvmovdqu64 %1,%%zmm0\n\tvmovdqu64 %2,%%zmm1\n\tvmovdqu64 %3,%%zmm2\n\tvpermq %%zmm0,%%zmm1,%%zmm2%{%%k1%}\n\tvmovdqu64 %%zmm2,%0"
        : "=m"(dst) : "m"(src), "m"(idx), "m"(initial), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == initial.u64[i], "EVPERMUTQ k=0 merge lane %d", i);
    kmask = 0xff;
    __asm__ volatile ("kmovq %3,%%k1\n\tvmovdqu64 %1,%%zmm0\n\tvmovdqu64 %2,%%zmm1\n\tvpermq %%zmm0,%%zmm1,%%zmm2%{%%k1%}%{z%}\n\tvmovdqu64 %%zmm2,%0"
        : "=m"(dst) : "m"(src), "m"(idx), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == src.u64[idx.u64[i] & 7], "EVPERMUTQ full mask lane %d", i);

    TEST_END();
}
