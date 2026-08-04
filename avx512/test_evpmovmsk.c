/*
 * Test VPCMPEQB/VPCMPEQD/VPCMPEQW producing k-regs as EVPMOVMSK* equivalents.
 * AVX-512 uses VPCMPEQ* -> k register + KMOVQ to read mask (replaces VPMOVMSKB etc.).
 *
 * Compile: gcc -o test_evpmovmsk avx512/test_evpmovmsk.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPMOVMSKB/D/W (VPCMPEQB/D/W zmm -> k + KMOVQ)");

    zmm_t a, b;
    uint64_t kmask;

    /* EVPMOVMSKB: compare all bytes equal to 0x80 (high bit set) */
    for (int i = 0; i < 64; i++) a.u8[i] = (i % 2 == 0) ? 0x80 : 0x00;
    for (int i = 0; i < 64; i++) b.u8[i] = 0x80;
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpcmpeqb %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0x5555555555555555ULL,
        "EVPMOVMSKB alternating: mask=%016llx", (unsigned long long)kmask);

    /* EVPMOVMSKB: all bytes equal */
    for (int i = 0; i < 64; i++) { a.u8[i] = 0xAA; b.u8[i] = 0xAA; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpcmpeqb %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0xFFFFFFFFFFFFFFFFULL,
        "EVPMOVMSKB all-equal: mask=%016llx", (unsigned long long)kmask);

    /* EVPMOVMSKB: no bytes equal */
    for (int i = 0; i < 64; i++) { a.u8[i] = 0x00; b.u8[i] = 0xFF; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpcmpeqb %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0, "EVPMOVMSKB none-equal: mask=%016llx", (unsigned long long)kmask);

    /* EVPMOVMSKD: compare dwords */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i % 4 == 0 ? 0xDEAD : i); b.u32[i] = 0xDEAD; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpcmpeqd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT((kmask & 0xFFFF) == 0x1111,
        "EVPMOVMSKD every4th: mask=%04llx", (unsigned long long)(kmask & 0xFFFF));

    /* EVPMOVMSKW: compare words */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i < 16 ? 0xBEEF : i); b.u16[i] = 0xBEEF; }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpcmpeqw %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT((kmask & 0xFFFFFFFF) == 0x0000FFFF,
        "EVPMOVMSKW lower16: mask=%08llx", (unsigned long long)(kmask & 0xFFFFFFFF));

    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)i; b.u8[i] = (uint8_t)(i + 1); }
    b.u8[0] = a.u8[0]; b.u8[63] = a.u8[63];
    __asm__ volatile ("vmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvpcmpeqb %%zmm1,%%zmm0,%%k1\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(kmask == (UINT64_C(1) | (UINT64_C(1) << 63)), "EVPMOVMSKB lowest/highest bits: %016llx", (unsigned long long)kmask);

    /* Effective result widths must clear unused high bits of the k register. */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)i; b.u16[i] = (uint16_t)(i + 1); }
    b.u16[0] = a.u16[0]; b.u16[31] = a.u16[31];
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\t"
        "vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\t"
        "vpcmpeqw %%zmm1,%%zmm0,%%k1\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(kmask == UINT64_C(0x0000000080000001),
                "VPCMPEQW clears unused upper k bits: %#" PRIx64, kmask);

    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)i; b.u32[i] = (uint32_t)(i + 1); }
    b.u32[0] = a.u32[0]; b.u32[15] = a.u32[15];
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\t"
        "vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\t"
        "vpcmpeqd %%zmm1,%%zmm0,%%k1\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(kmask == UINT64_C(0x8001),
                "VPCMPEQD exact k width and endpoint bits: %#" PRIx64, kmask);

    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)i; b.u64[i] = (uint64_t)(i + 1); }
    b.u64[0] = a.u64[0]; b.u64[7] = a.u64[7];
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\t"
        "vmovdqu64 %1,%%zmm0\n\tvmovdqu64 %2,%%zmm1\n\t"
        "vpcmpeqq %%zmm1,%%zmm0,%%k1\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(kmask == UINT64_C(0x81),
                "VPCMPEQQ exact k width and endpoint bits: %#" PRIx64, kmask);

    /* AVX-512VL widths also clear all k bits above the active vector lanes. */
    for (int i = 0; i < 16; i++) { a.u8[i] = (uint8_t)i; b.u8[i] = (uint8_t)(i + 1); }
    b.u8[0] = a.u8[0]; b.u8[15] = a.u8[15];
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\t"
        "vmovdqu8 %1,%%xmm0\n\tvpcmpeqb %2,%%xmm0,%%k1\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "xmm0", "k1");
    TEST_ASSERT(kmask == UINT64_C(0x8001), "VPCMPEQB xmm exact mask width");

    for (int i = 0; i < 4; i++) { a.u64[i] = (uint64_t)i; b.u64[i] = (uint64_t)(i + 1); }
    b.u64[0] = a.u64[0]; b.u64[3] = a.u64[3];
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\t"
        "vmovdqu64 %1,%%ymm0\n\tvpcmpeqq %2,%%ymm0,%%k1\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "ymm0", "k1");
    TEST_ASSERT(kmask == UINT64_C(0x9), "VPCMPEQQ ymm exact mask width");

    /* A compare destination writemask zeroes disabled result bits. */
    for (int i = 0; i < 16; i++) { a.u32[i] = 42; b.u32[i] = 42; }
    uint64_t gate = 0;
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\tkmovq %3,%%k2\n\t"
        "vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\t"
        "vpcmpeqd %%zmm1,%%zmm0,%%k1%{%%k2%}\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b), "r"(gate)
        : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT(kmask == 0, "VPCMPEQD k=0 result writemask clears destination k");
    gate = UINT64_C(0x8001);
    __asm__ volatile (
        "kmovq %3,%%k2\n\tvmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\t"
        "vpcmpeqd %%zmm1,%%zmm0,%%k1%{%%k2%}\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b), "r"(gate)
        : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT(kmask == UINT64_C(0x8001),
                "VPCMPEQD endpoint result writemask and exact k width");

    TEST_END();
}
