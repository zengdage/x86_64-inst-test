/*
 * Test VPADDB/VPADDW/VPADDD/VPADDQ with zmm registers (AVX-512BW/F).
 * Packed integer addition (byte/word/dword/qword).
 *
 * Compile: gcc -o test_evpaddb_w_d_q avx512/test_evpaddb_w_d_q.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1); /* AVX512F + AVX512BW */
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512F/BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPADDB/EVPADDW/EVPADDD/EVPADDQ (VPADDB/VPADDW/VPADDD/VPADDQ zmm)");

    zmm_t a, b, dst;

    /* VPADDB: 64 bytes */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)i; b.u8[i] = (uint8_t)(64 - i); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpaddb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == (uint8_t)(a.u8[i] + b.u8[i]),
            "VPADDB lane %d: %u != %u", i, dst.u8[i], (uint8_t)(a.u8[i]+b.u8[i]));

    /* VPADDW: 32 words */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i * 100); b.u16[i] = (uint16_t)(i * 50 + 1); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpaddw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == (uint16_t)(a.u16[i] + b.u16[i]),
            "VPADDW lane %d: %u != %u", i, dst.u16[i], (uint16_t)(a.u16[i]+b.u16[i]));

    /* VPADDD: 16 dwords */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i * 1000); b.u32[i] = (uint32_t)(i * 500 + 7); }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpaddd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == a.u32[i] + b.u32[i],
            "VPADDD lane %d: %u != %u", i, dst.u32[i], a.u32[i]+b.u32[i]);

    /* VPADDQ: 8 qwords */
    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)i * 0x100000000ULL; b.u64[i] = (uint64_t)(i + 1); }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpaddq %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == a.u64[i] + b.u64[i],
            "VPADDQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)(a.u64[i]+b.u64[i]));

    /* VPADDD with zeroing masking {k1}{z} */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i + 1); b.u32[i] = 10; }
    uint64_t kmask = 0xFF00;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpaddd %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? a.u32[i] + 10 : 0;
        TEST_ASSERT(dst.u32[i] == expected,
            "VPADDD zero mask lane %d: %u != %u", i, dst.u32[i], expected);
    }

    /* Boundary: all-ones wrap */
    memset(&a, 0xFF, sizeof(a)); memset(&b, 0, sizeof(b)); b.u8[0] = 1;
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpaddb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    TEST_ASSERT(dst.u8[0] == 0, "VPADDB wrap lane 0: %u", dst.u8[0]);

    /* Wraparound at every element width. */
#define TEST_ADD_WRAP(TYPE, FIELD, MOVE, INSN, LANES) do { \
        for (int i = 0; i < (LANES); i++) { a.FIELD[i] = (TYPE)~(TYPE)0; b.FIELD[i] = 1; } \
        __asm__ volatile (MOVE " %1, %%zmm0\n\t" MOVE " %2, %%zmm1\n\t" \
                          INSN " %%zmm1, %%zmm0, %%zmm2\n\t" MOVE " %%zmm2, %0" \
                          : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"); \
        for (int i = 0; i < (LANES); i++) \
            TEST_ASSERT(dst.FIELD[i] == 0, INSN " wrap lane %d", i); \
    } while (0)
    TEST_ADD_WRAP(uint16_t, u16, "vmovdqu16", "vpaddw", 32);
    TEST_ADD_WRAP(uint32_t, u32, "vmovdqu32", "vpaddd", 16);
    TEST_ADD_WRAP(uint64_t, u64, "vmovdqu64", "vpaddq", 8);
#undef TEST_ADD_WRAP

    /* Dword mask boundaries: k=0 merge and endpoint-only zero mask. */
    for (int i = 0; i < 16; i++) { a.u32[i] = 10; b.u32[i] = 20; dst.u32[i] = UINT32_C(0xdeadbeef); }
    kmask = 0;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t" "vmovdqu32 %0, %%zmm2\n\t"
        "vmovdqu32 %1, %%zmm0\n\t" "vmovdqu32 %2, %%zmm1\n\t"
        "vpaddd %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t" "vmovdqu32 %%zmm2, %0"
        : "+m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == UINT32_C(0xdeadbeef), "VPADDD k=0 merge lane %d", i);

    kmask = UINT64_C(0x8001);
    __asm__ volatile (
        "kmovq %3, %%k1\n\t" "vmovdqu32 %1, %%zmm0\n\t" "vmovdqu32 %2, %%zmm1\n\t"
        "vpaddd %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t" "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == ((i == 0 || i == 15) ? 30U : 0U), "VPADDD endpoint zero mask lane %d", i);

    TEST_END();
}
