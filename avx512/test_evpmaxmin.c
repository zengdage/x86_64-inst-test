/*
 * Test VPMAXSB/VPMAXSW/VPMAXSD/VPMAXSQ/VPMAXUB/VPMAXUW/VPMAXUD/VPMAXUQ and
 *      VPMINSB/VPMINSW/VPMINSD/VPMINSQ/VPMINUB/VPMINUW/VPMINUD/VPMINUQ with zmm (AVX-512BW/F/DQ).
 * Packed integer max/min (signed and unsigned).
 *
 * Compile: gcc -o test_evpmaxmin avx512/test_evpmaxmin.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512F/BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPMAXSB/SW/SD/SQ/UB/UW/UD/UQ + EVPMINSB/SW/SD/SQ/UB/UW/UD/UQ (zmm)");

    zmm_t a, b, dst;

    /* VPMAXSB */
    for (int i = 0; i < 64; i++) { a.i8[i] = (int8_t)(i - 32); b.i8[i] = (int8_t)(31 - i); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t" "vmovdqu8 %2, %%zmm1\n\t"
        "vpmaxsb %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 64; i++) {
        int8_t e = a.i8[i] > b.i8[i] ? a.i8[i] : b.i8[i];
        TEST_ASSERT(dst.i8[i] == e, "VPMAXSB lane %d: %d != %d", i, dst.i8[i], e);
    }

    /* VPMINSB */
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t" "vmovdqu8 %2, %%zmm1\n\t"
        "vpminsb %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 64; i++) {
        int8_t e = a.i8[i] < b.i8[i] ? a.i8[i] : b.i8[i];
        TEST_ASSERT(dst.i8[i] == e, "VPMINSB lane %d: %d != %d", i, dst.i8[i], e);
    }

    /* VPMAXSW */
    for (int i = 0; i < 32; i++) { a.i16[i] = (int16_t)(i * 100 - 1600); b.i16[i] = (int16_t)(1500 - i * 100); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t" "vmovdqu16 %2, %%zmm1\n\t"
        "vpmaxsw %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 32; i++) {
        int16_t e = a.i16[i] > b.i16[i] ? a.i16[i] : b.i16[i];
        TEST_ASSERT(dst.i16[i] == e, "VPMAXSW lane %d: %d != %d", i, dst.i16[i], e);
    }

    /* VPMINSD */
    for (int i = 0; i < 16; i++) { a.i32[i] = (int)(i * 1000 - 8000); b.i32[i] = (int)(7000 - i * 1000); }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t" "vmovdqu32 %2, %%zmm1\n\t"
        "vpminsd %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 16; i++) {
        int e = a.i32[i] < b.i32[i] ? a.i32[i] : b.i32[i];
        TEST_ASSERT(dst.i32[i] == e, "VPMINSD lane %d: %d != %d", i, dst.i32[i], e);
    }

    /* VPMAXSQ */
    for (int i = 0; i < 8; i++) { a.i64[i] = (long long)(i * 1000000 - 4000000); b.i64[i] = (long long)(3000000 - i * 1000000); }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t" "vmovdqu64 %2, %%zmm1\n\t"
        "vpmaxsq %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 8; i++) {
        long long e = a.i64[i] > b.i64[i] ? a.i64[i] : b.i64[i];
        TEST_ASSERT(dst.i64[i] == e, "VPMAXSQ lane %d: %lld != %lld", i, (long long)dst.i64[i], e);
    }

    /* VPMAXUB */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i * 4); b.u8[i] = (uint8_t)(255 - i * 4); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t" "vmovdqu8 %2, %%zmm1\n\t"
        "vpmaxub %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 64; i++) {
        uint8_t e = a.u8[i] > b.u8[i] ? a.u8[i] : b.u8[i];
        TEST_ASSERT(dst.u8[i] == e, "VPMAXUB lane %d: %u != %u", i, dst.u8[i], e);
    }

    /* VPMINUW */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i * 2000); b.u16[i] = (uint16_t)(62000 - i * 2000); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t" "vmovdqu16 %2, %%zmm1\n\t"
        "vpminuw %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 32; i++) {
        uint16_t e = a.u16[i] < b.u16[i] ? a.u16[i] : b.u16[i];
        TEST_ASSERT(dst.u16[i] == e, "VPMINUW lane %d: %u != %u", i, dst.u16[i], e);
    }

    /* VPMAXUD */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i * 100000); b.u32[i] = (uint32_t)(1500000 - i * 100000); }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t" "vmovdqu32 %2, %%zmm1\n\t"
        "vpmaxud %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 16; i++) {
        uint32_t e = a.u32[i] > b.u32[i] ? a.u32[i] : b.u32[i];
        TEST_ASSERT(dst.u32[i] == e, "VPMAXUD lane %d: %u != %u", i, dst.u32[i], e);
    }

    /* VPMINUQ */
    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)(i + 1) * 1000000000ULL; b.u64[i] = (uint64_t)(9 - i) * 1000000000ULL; }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t" "vmovdqu64 %2, %%zmm1\n\t"
        "vpminuq %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2");
    for (int i = 0; i < 8; i++) {
        uint64_t e = a.u64[i] < b.u64[i] ? a.u64[i] : b.u64[i];
        TEST_ASSERT(dst.u64[i] == e, "VPMINUQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)e);
    }

    /* Previously missing signed/unsigned variants, with exact extrema and ties. */
    for (int i = 0; i < 32; i++) {
        a.i16[i] = (i % 3 == 0) ? INT16_MIN : (i % 3 == 1 ? INT16_MAX : 7);
        b.i16[i] = (i % 3 == 0) ? INT16_MAX : (i % 3 == 1 ? INT16_MIN : 7);
    }
    __asm__ volatile ("vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpminsw %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu16 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 32; i++) TEST_ASSERT(dst.i16[i] == (a.i16[i] < b.i16[i] ? a.i16[i] : b.i16[i]), "VPMINSW extrema lane %d", i);

    for (int i = 0; i < 16; i++) {
        a.i32[i] = (i % 3 == 0) ? INT32_MIN : (i % 3 == 1 ? INT32_MAX : -9);
        b.i32[i] = (i % 3 == 0) ? INT32_MAX : (i % 3 == 1 ? INT32_MIN : -9);
    }
    __asm__ volatile ("vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvpmaxsd %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu32 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.i32[i] == (a.i32[i] > b.i32[i] ? a.i32[i] : b.i32[i]), "VPMAXSD extrema lane %d", i);

    for (int i = 0; i < 8; i++) {
        a.i64[i] = (i % 3 == 0) ? INT64_MIN : (i % 3 == 1 ? INT64_MAX : 11);
        b.i64[i] = (i % 3 == 0) ? INT64_MAX : (i % 3 == 1 ? INT64_MIN : 11);
    }
    __asm__ volatile ("vmovdqu64 %1,%%zmm0\n\tvmovdqu64 %2,%%zmm1\n\tvpminsq %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu64 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.i64[i] == (a.i64[i] < b.i64[i] ? a.i64[i] : b.i64[i]), "VPMINSQ extrema lane %d", i);

    for (int i = 0; i < 64; i++) { a.u8[i] = (i % 3 == 0) ? 0 : (i % 3 == 1 ? UINT8_MAX : 42); b.u8[i] = (i % 3 == 0) ? UINT8_MAX : (i % 3 == 1 ? 0 : 42); }
    __asm__ volatile ("vmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvpminub %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 64; i++) TEST_ASSERT(dst.u8[i] == (a.u8[i] < b.u8[i] ? a.u8[i] : b.u8[i]), "VPMINUB extrema lane %d", i);

    for (int i = 0; i < 32; i++) { a.u16[i] = (i % 3 == 0) ? 0 : (i % 3 == 1 ? UINT16_MAX : 99); b.u16[i] = (i % 3 == 0) ? UINT16_MAX : (i % 3 == 1 ? 0 : 99); }
    __asm__ volatile ("vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpmaxuw %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu16 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 32; i++) TEST_ASSERT(dst.u16[i] == (a.u16[i] > b.u16[i] ? a.u16[i] : b.u16[i]), "VPMAXUW extrema lane %d", i);

    for (int i = 0; i < 16; i++) { a.u32[i] = (i % 3 == 0) ? 0 : (i % 3 == 1 ? UINT32_MAX : 123); b.u32[i] = (i % 3 == 0) ? UINT32_MAX : (i % 3 == 1 ? 0 : 123); }
    __asm__ volatile ("vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvpminud %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu32 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == (a.u32[i] < b.u32[i] ? a.u32[i] : b.u32[i]), "VPMINUD extrema lane %d", i);

    for (int i = 0; i < 8; i++) { a.u64[i] = (i % 3 == 0) ? 0 : (i % 3 == 1 ? UINT64_MAX : 321); b.u64[i] = (i % 3 == 0) ? UINT64_MAX : (i % 3 == 1 ? 0 : 321); }
    __asm__ volatile ("vmovdqu64 %1,%%zmm0\n\tvmovdqu64 %2,%%zmm1\n\tvpmaxuq %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu64 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == (a.u64[i] > b.u64[i] ? a.u64[i] : b.u64[i]), "VPMAXUQ extrema lane %d", i);

    zmm_t initial;
    for (int i = 0; i < 16; i++) { a.i32[i] = i; b.i32[i] = 100 + i; initial.i32[i] = -777; }
    uint64_t kmask = 0x8001;
    __asm__ volatile ("kmovq %4,%%k1\n\tvmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvmovdqu32 %3,%%zmm2\n\tvpmaxsd %%zmm1,%%zmm0,%%zmm2%{%%k1%}\n\tvmovdqu32 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(initial), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.i32[i] == ((i == 0 || i == 15) ? b.i32[i] : -777), "VPMAXSD merge mask lane %d", i);

    TEST_END();
}
