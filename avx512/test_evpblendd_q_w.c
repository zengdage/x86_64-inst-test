/*
 * Test VPBLENDD/VPBLENDQ/VPBLENDW — note: VPBLENDD/VPBLENDW are VEX-only (AVX2),
 * no AVX-512 zmm form. Implemented using ymm (256-bit) with VEX encoding.
 * VPBLENDQ does not exist; use VPBLENDMQ (AVX-512) for zmm blend.
 *
 * Compile: gcc -o test_evpblendd_q_w avx512/test_evpblendd_q_w.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

#define RUN_BLENDD(imm, out, lhs, rhs) do { \
    __asm__ volatile ( \
        "vmovdqu %1, %%ymm0\n\t" \
        "vmovdqu %2, %%ymm1\n\t" \
        "vpblendd $" #imm ", %%ymm1, %%ymm0, %%ymm2\n\t" \
        "vmovdqu %%ymm2, %0" \
        : "=m"(out) : "m"(lhs), "m"(rhs) : "ymm0", "ymm1", "ymm2"); \
} while (0)

#define RUN_BLENDW(imm, out, lhs, rhs) do { \
    __asm__ volatile ( \
        "vmovdqu %1, %%ymm0\n\t" \
        "vmovdqu %2, %%ymm1\n\t" \
        "vpblendw $" #imm ", %%ymm1, %%ymm0, %%ymm2\n\t" \
        "vmovdqu %%ymm2, %0" \
        : "=m"(out) : "m"(lhs), "m"(rhs) : "ymm0", "ymm1", "ymm2"); \
} while (0)

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 5) & 1;
}
#else
#define check_avx2() 1
#endif
#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 16) & 1;
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx2()) {
        printf("AVX2 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPBLENDD/EVPBLENDW (ymm VEX) + EVPBLENDMQ (zmm AVX-512)");

    ymm_t a, b, dst;

    /* VPBLENDD ymm: imm8=0b10101010 -> odd dwords from b, even from a */
    for (int i = 0; i < 8; i++) { a.u32[i] = (uint32_t)(i + 1) * 10; b.u32[i] = (uint32_t)(i + 1) * 100; }
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpblendd $0xAA, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++) {
        uint32_t expected = (0xAA >> i) & 1 ? b.u32[i] : a.u32[i];
        TEST_ASSERT(dst.u32[i] == expected,
            "VPBLENDD lane %d: %u != %u", i, dst.u32[i], expected);
    }

    RUN_BLENDD(0x00, dst, a, b);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u32[i] == a.u32[i], "VPBLENDD none lane %d", i);
    RUN_BLENDD(0xff, dst, a, b);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u32[i] == b.u32[i], "VPBLENDD all lane %d", i);
    RUN_BLENDD(0x01, dst, a, b);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u32[i] == (i == 0 ? b.u32[i] : a.u32[i]), "VPBLENDD low lane boundary %d", i);
    RUN_BLENDD(0x80, dst, a, b);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u32[i] == (i == 7 ? b.u32[i] : a.u32[i]), "VPBLENDD high lane boundary %d", i);

    /* VPBLENDW ymm: imm8=0b11001100 -> blend words */
    for (int i = 0; i < 16; i++) { a.u16[i] = (uint16_t)(i + 1); b.u16[i] = (uint16_t)(i + 100); }
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpblendw $0xCC, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    /* VPBLENDW imm applies per 8-word group */
    for (int g = 0; g < 2; g++) {
        for (int j = 0; j < 8; j++) {
            uint16_t expected = (0xCC >> j) & 1 ? b.u16[g*8+j] : a.u16[g*8+j];
            TEST_ASSERT(dst.u16[g*8+j] == expected,
                "VPBLENDW g%d lane%d: %u != %u", g, j, dst.u16[g*8+j], expected);
        }
    }

    RUN_BLENDW(0x00, dst, a, b);
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u16[i] == a.u16[i], "VPBLENDW none lane %d", i);
    RUN_BLENDW(0xff, dst, a, b);
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u16[i] == b.u16[i], "VPBLENDW all lane %d", i);
    RUN_BLENDW(0x01, dst, a, b);
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u16[i] == ((i % 8) == 0 ? b.u16[i] : a.u16[i]), "VPBLENDW low lane per half %d", i);
    RUN_BLENDW(0x80, dst, a, b);
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u16[i] == ((i % 8) == 7 ? b.u16[i] : a.u16[i]), "VPBLENDW high lane per half %d", i);

    /* VPBLENDMQ zmm (AVX-512): blend qwords using mask */
    if (check_avx512()) {
        zmm_t za, zb, zdst;
        uint64_t kmask = 0xAA; /* alternating */
        for (int i = 0; i < 8; i++) { za.u64[i] = (uint64_t)(i + 1) * 10; zb.u64[i] = (uint64_t)(i + 1) * 100; }
        __asm__ volatile (
            "kmovq %3, %%k1\n\t"
            "vmovdqu64 %1, %%zmm0\n\t"
            "vmovdqu64 %2, %%zmm1\n\t"
            "vpblendmq %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\t"
            "vmovdqu64 %%zmm2, %0"
            : "=m"(zdst) : "m"(za), "m"(zb), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
        );
        for (int i = 0; i < 8; i++) {
            uint64_t expected = (kmask >> i) & 1 ? za.u64[i] : zb.u64[i];
            TEST_ASSERT(zdst.u64[i] == expected,
                "VPBLENDMQ lane %d: %llu != %llu", i,
                (unsigned long long)zdst.u64[i], (unsigned long long)expected);
        }
        const uint64_t masks[] = {0x00, 0xff, 0x01, 0x80};
        for (unsigned m = 0; m < sizeof(masks) / sizeof(masks[0]); m++) {
            kmask = masks[m];
            __asm__ volatile (
                "kmovq %3, %%k1\n\t"
                "vmovdqu64 %1, %%zmm0\n\t"
                "vmovdqu64 %2, %%zmm1\n\t"
                "vpblendmq %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\t"
                "vmovdqu64 %%zmm2, %0"
                : "=m"(zdst) : "m"(za), "m"(zb), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1"
            );
            for (int i = 0; i < 8; i++) {
                uint64_t expected = (kmask >> i) & 1 ? za.u64[i] : zb.u64[i];
                TEST_ASSERT(zdst.u64[i] == expected, "VPBLENDMQ mask 0x%02llx lane %d",
                    (unsigned long long)kmask, i);
            }
        }
    }

    TEST_END();
}
