/*
 * Test VPCMPEQB/VPCMPEQW/VPCMPEQD/VPCMPEQQ with zmm -> k register (AVX-512BW/F).
 * Packed compare equal producing mask results.
 *
 * Compile: gcc -o test_evpcmpeq avx512/test_evpcmpeq.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPCMPEQB/EVPCMPEQW/EVPCMPEQD/EVPCMPEQQ (zmm -> k)");

    zmm_t a, b;
    uint64_t kmask;

    /* VPCMPEQD: all equal */
    for (int i = 0; i < 16; i++) { a.u32[i] = 42; b.u32[i] = 42; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpcmpeqd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0xFFFF, "VPCMPEQD all-equal: mask=%04llx", (unsigned long long)kmask);

    /* VPCMPEQD: none equal */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)i; b.u32[i] = (uint32_t)(i + 1); }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpcmpeqd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0, "VPCMPEQD none-equal: mask=%04llx", (unsigned long long)kmask);

    /* VPCMPEQD: alternating equal */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)i; b.u32[i] = (i % 2 == 0) ? (uint32_t)i : (uint32_t)(i + 1); }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpcmpeqd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0x5555, "VPCMPEQD alternating: mask=%04llx", (unsigned long long)kmask);

    /* VPCMPEQQ: 8 qword lanes */
    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)(i * 100); b.u64[i] = (i < 4) ? a.u64[i] : a.u64[i] + 1; }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpcmpeqq %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0x0F, "VPCMPEQQ lower4-equal: mask=%02llx", (unsigned long long)kmask);

    /* VPCMPEQB: 64 byte lanes */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)i; b.u8[i] = (i < 32) ? (uint8_t)i : (uint8_t)(i + 1); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpcmpeqb %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0x00000000FFFFFFFFULL, "VPCMPEQB lower32-equal: mask=%016llx", (unsigned long long)kmask);

    /* VPCMPEQW: 32 word lanes */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i * 10); b.u16[i] = (i % 2 == 0) ? a.u16[i] : (uint16_t)(a.u16[i] + 1); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpcmpeqw %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    uint32_t expected_mask = 0;
    for (int i = 0; i < 32; i++) if (i % 2 == 0) expected_mask |= (1u << i);
    TEST_ASSERT((kmask & 0xFFFFFFFF) == expected_mask, "VPCMPEQW alternating: mask=%08llx", (unsigned long long)kmask);

    /* Endpoint result bits, k=0 zeroing, and exact valid mask widths.  Fill
       k1 with ones before each compare so stale high bits are observable. */
#define TEST_CMPEQ_MASK_BOUNDARY(FIELD, MOVE, INSN, LANES, VALID, ENDPOINTS) do { \
        uint64_t gate; \
        for (int i = 0; i < (LANES); i++) { a.FIELD[i] = 0; b.FIELD[i] = 1; } \
        b.FIELD[0] = 0; b.FIELD[(LANES) - 1] = 0; \
        __asm__ volatile ( \
            "kxnorq %%k1,%%k1,%%k1\n\t" MOVE " %1,%%zmm0\n\t" \
            MOVE " %2,%%zmm1\n\t" INSN " %%zmm1,%%zmm0,%%k1\n\t" \
            "kmovq %%k1,%0" \
            : "=r"(kmask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1"); \
        TEST_ASSERT(kmask == (ENDPOINTS), \
                    INSN " lowest/highest result bits and high-bit clearing: %016llx", \
                    (unsigned long long)kmask); \
        for (int i = 0; i < (LANES); i++) b.FIELD[i] = a.FIELD[i]; \
        gate = 0; \
        __asm__ volatile ( \
            "kxnorq %%k1,%%k1,%%k1\n\tkmovq %3,%%k2\n\t" \
            MOVE " %1,%%zmm0\n\t" MOVE " %2,%%zmm1\n\t" \
            INSN " %%zmm1,%%zmm0,%%k1%{%%k2%}\n\tkmovq %%k1,%0" \
            : "=r"(kmask) : "m"(a), "m"(b), "r"(gate) \
            : "zmm0", "zmm1", "k1", "k2"); \
        TEST_ASSERT(kmask == 0, INSN " k=0 clears the complete result mask"); \
        gate = (VALID); \
        __asm__ volatile ( \
            "kxnorq %%k1,%%k1,%%k1\n\tkmovq %3,%%k2\n\t" \
            MOVE " %1,%%zmm0\n\t" MOVE " %2,%%zmm1\n\t" \
            INSN " %%zmm1,%%zmm0,%%k1%{%%k2%}\n\tkmovq %%k1,%0" \
            : "=r"(kmask) : "m"(a), "m"(b), "r"(gate) \
            : "zmm0", "zmm1", "k1", "k2"); \
        TEST_ASSERT(kmask == (VALID), INSN " k=all exact valid mask width: %016llx", \
                    (unsigned long long)kmask); \
    } while (0)
    TEST_CMPEQ_MASK_BOUNDARY(u8,  "vmovdqu8",  "vpcmpeqb", 64,
                            UINT64_MAX, UINT64_C(0x8000000000000001));
    TEST_CMPEQ_MASK_BOUNDARY(u16, "vmovdqu16", "vpcmpeqw", 32,
                            UINT64_C(0xffffffff), UINT64_C(0x80000001));
    TEST_CMPEQ_MASK_BOUNDARY(u32, "vmovdqu32", "vpcmpeqd", 16,
                            UINT64_C(0xffff), UINT64_C(0x8001));
    TEST_CMPEQ_MASK_BOUNDARY(u64, "vmovdqu64", "vpcmpeqq", 8,
                            UINT64_C(0xff), UINT64_C(0x81));
#undef TEST_CMPEQ_MASK_BOUNDARY

    /* AVX-512VL uses narrower architectural result masks for XMM/YMM. */
    for (int i = 0; i < 8; i++) { a.u32[i] = (uint32_t)i; b.u32[i] = a.u32[i]; }
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\tvmovdqu32 %1,%%xmm0\n\t"
        "vmovdqu32 %2,%%xmm1\n\tvpcmpeqd %%xmm1,%%xmm0,%%k1\n\t"
        "kmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "xmm0", "xmm1", "k1");
    TEST_ASSERT(kmask == UINT64_C(0x0f),
                "VPCMPEQD xmm clears bits above four valid lanes: %016llx",
                (unsigned long long)kmask);
    b.u32[0] ^= 1;
    __asm__ volatile (
        "kxnorq %%k1,%%k1,%%k1\n\tvmovdqu32 %1,%%ymm0\n\t"
        "vmovdqu32 %2,%%ymm1\n\tvpcmpeqd %%ymm1,%%ymm0,%%k1\n\t"
        "kmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b) : "ymm0", "ymm1", "k1");
    TEST_ASSERT(kmask == UINT64_C(0xfe),
                "VPCMPEQD ymm result width and lowest-lane boundary: %016llx",
                (unsigned long long)kmask);

    TEST_END();
}
