/*
 * Test VPABSB/VPABSW/VPABSD/VPABSQ with zmm registers (AVX-512BW/F/DQ).
 * Packed absolute value (byte/word/dword/qword).
 *
 * Compile: gcc -o test_evpabs avx512/test_evpabs.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
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
    TEST_START("EVPABSB/EVPABSW/EVPABSD/EVPABSQ (VPABSB/VPABSW/VPABSD/VPABSQ zmm)");

    zmm_t src, dst;

    /* VPABSB */
    for (int i = 0; i < 64; i++) src.i8[i] = (int8_t)(i - 32);
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vpabsb %%zmm0, %%zmm1\n\t"
        "vmovdqu8 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int i = 0; i < 64; i++) {
        uint8_t expected = src.i8[i] < 0 ? (uint8_t)(-(int)src.i8[i]) : (uint8_t)src.i8[i];
        TEST_ASSERT(dst.u8[i] == expected, "VPABSB lane %d: %u != %u", i, dst.u8[i], expected);
    }

    /* VPABSW */
    for (int i = 0; i < 32; i++) src.i16[i] = (int16_t)(i * 100 - 1600);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vpabsw %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int i = 0; i < 32; i++) {
        uint16_t expected = src.i16[i] < 0 ? (uint16_t)(-(int)src.i16[i]) : (uint16_t)src.i16[i];
        TEST_ASSERT(dst.u16[i] == expected, "VPABSW lane %d: %u != %u", i, dst.u16[i], expected);
    }

    /* VPABSD */
    for (int i = 0; i < 16; i++) src.i32[i] = (int)(i * 1000 - 8000);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpabsd %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = src.i32[i] < 0 ? (uint32_t)(-(long long)src.i32[i]) : (uint32_t)src.i32[i];
        TEST_ASSERT(dst.u32[i] == expected, "VPABSD lane %d: %u != %u", i, dst.u32[i], expected);
    }

    /* VPABSQ */
    for (int i = 0; i < 8; i++) src.i64[i] = (long long)(i * 1000000 - 4000000);
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vpabsq %%zmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int i = 0; i < 8; i++) {
        uint64_t expected = src.i64[i] < 0 ? (uint64_t)(-src.i64[i]) : (uint64_t)src.i64[i];
        TEST_ASSERT(dst.u64[i] == expected, "VPABSQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)expected);
    }

    /* Boundary: abs(0) = 0 */
    memset(&src, 0, sizeof(src));
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpabsd %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0, "VPABSD zero lane %d: %u", i, dst.u32[i]);

    /* VPABSD with zeroing masking */
    for (int i = 0; i < 16; i++) src.i32[i] = -(i + 1);
    uint64_t kmask = 0x5555;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpabsd %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(src), "m"(src), "r"(kmask) : "zmm0","zmm1","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? (uint32_t)(i + 1) : 0;
        TEST_ASSERT(dst.u32[i] == expected, "VPABSD zero mask lane %d: %u != %u", i, dst.u32[i], expected);
    }

    /* INT_MIN cannot be represented as a positive signed value; the bit pattern is preserved. */
#define TEST_ABS_MIN(FIELD, MOVE, INSN, LANES, MINVAL, EXPECTED) do { \
        for (int i = 0; i < (LANES); i++) src.FIELD[i] = (MINVAL); \
        __asm__ volatile (MOVE " %1, %%zmm0\n\t" INSN " %%zmm0, %%zmm1\n\t" \
                          MOVE " %%zmm1, %0" : "=m"(dst) : "m"(src) : "zmm0","zmm1"); \
        for (int i = 0; i < (LANES); i++) \
            TEST_ASSERT(dst.FIELD[i] == (EXPECTED), INSN " INT_MIN lane %d", i); \
    } while (0)
    TEST_ABS_MIN(u8,  "vmovdqu8",  "vpabsb", 64, INT8_MIN,  UINT8_C(0x80));
    TEST_ABS_MIN(u16, "vmovdqu16", "vpabsw", 32, INT16_MIN, UINT16_C(0x8000));
    TEST_ABS_MIN(u32, "vmovdqu32", "vpabsd", 16, INT32_MIN, UINT32_C(0x80000000));
    TEST_ABS_MIN(u64, "vmovdqu64", "vpabsq", 8,  INT64_MIN, UINT64_C(0x8000000000000000));
#undef TEST_ABS_MIN

    /* Lowest/highest active dword mask bits with merge semantics. */
    for (int i = 0; i < 16; i++) { src.i32[i] = -(i + 1); dst.u32[i] = UINT32_C(0xcccccccc); }
    kmask = UINT64_C(0x8001);
    __asm__ volatile (
        "kmovq %2, %%k1\n\t" "vmovdqu32 %0, %%zmm2\n\t" "vmovdqu32 %1, %%zmm0\n\t"
        "vpabsd %%zmm0, %%zmm2%{%%k1%}\n\t" "vmovdqu32 %%zmm2, %0"
        : "+m"(dst) : "m"(src), "r"(kmask) : "zmm0","zmm2","k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (i == 0 || i == 15) ? (uint32_t)(i + 1) : UINT32_C(0xcccccccc);
        TEST_ASSERT(dst.u32[i] == expected, "VPABSD endpoint merge mask lane %d", i);
    }

    TEST_END();
}
