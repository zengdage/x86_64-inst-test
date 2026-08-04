/*
 * Test VPADDSB/VPADDSW/VPADDUSB/VPADDUSW with zmm registers (AVX-512BW).
 * Packed integer saturating addition (signed/unsigned byte/word).
 *
 * Compile: gcc -o test_evpadds avx512/test_evpadds.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
static int check_avx512bw(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}
#else
#define check_avx512bw() 1
#endif

static int8_t sat_add_i8(int8_t a, int8_t b) {
    int r = (int)a + b;
    if (r > 127) return 127;
    if (r < -128) return -128;
    return (int8_t)r;
}
static int16_t sat_add_i16(int16_t a, int16_t b) {
    int r = (int)a + b;
    if (r > 32767) return 32767;
    if (r < -32768) return -32768;
    return (int16_t)r;
}
static uint8_t sat_add_u8(uint8_t a, uint8_t b) {
    unsigned r = (unsigned)a + b;
    return r > 255 ? 255 : (uint8_t)r;
}
static uint16_t sat_add_u16(uint16_t a, uint16_t b) {
    unsigned r = (unsigned)a + b;
    return r > 65535 ? 65535 : (uint16_t)r;
}

int main(void) {
    if (!check_avx512bw()) {
        printf("AVX-512BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPADDSB/EVPADDSW/EVPADDUSB/EVPADDUSW (zmm)");

    zmm_t a, b, dst;

    /* VPADDSB: signed byte saturating add */
    for (int i = 0; i < 64; i++) { a.i8[i] = (int8_t)(i - 32); b.i8[i] = (int8_t)(100); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpaddsb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.i8[i] == sat_add_i8(a.i8[i], b.i8[i]),
            "VPADDSB lane %d: %d != %d", i, dst.i8[i], sat_add_i8(a.i8[i], b.i8[i]));

    /* VPADDSW: signed word saturating add */
    for (int i = 0; i < 32; i++) { a.i16[i] = (int16_t)(i * 1000 - 16000); b.i16[i] = 30000; }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpaddsw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.i16[i] == sat_add_i16(a.i16[i], b.i16[i]),
            "VPADDSW lane %d: %d != %d", i, dst.i16[i], sat_add_i16(a.i16[i], b.i16[i]));

    /* VPADDUSB: unsigned byte saturating add */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i * 4); b.u8[i] = 200; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpaddusb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == sat_add_u8(a.u8[i], b.u8[i]),
            "VPADDUSB lane %d: %u != %u", i, dst.u8[i], sat_add_u8(a.u8[i], b.u8[i]));

    /* VPADDUSW: unsigned word saturating add */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i * 2000); b.u16[i] = 60000; }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpaddusw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == sat_add_u16(a.u16[i], b.u16[i]),
            "VPADDUSW lane %d: %u != %u", i, dst.u16[i], sat_add_u16(a.u16[i], b.u16[i]));

    /* Boundary: max + 1 saturates */
    memset(&a, 0x7F, sizeof(a)); memset(&b, 0x01, sizeof(b));
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpaddsb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    TEST_ASSERT(dst.i8[0] == 127, "VPADDSB max sat: %d", dst.i8[0]);

    a.i8[0] = INT8_MAX; b.i8[0] = 1;
    a.i8[1] = INT8_MIN; b.i8[1] = -1;
    a.i8[2] = INT8_MAX - 1; b.i8[2] = 1;
    a.i8[3] = INT8_MIN + 1; b.i8[3] = -1;
    __asm__ volatile ("vmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvpaddsb %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(dst.i8[0] == INT8_MAX && dst.i8[1] == INT8_MIN && dst.i8[2] == INT8_MAX && dst.i8[3] == INT8_MIN,
        "VPADDSB exact saturation thresholds");

    a.i16[0] = INT16_MAX; b.i16[0] = 1; a.i16[1] = INT16_MIN; b.i16[1] = -1;
    a.i16[2] = INT16_MAX - 1; b.i16[2] = 1; a.i16[3] = INT16_MIN + 1; b.i16[3] = -1;
    __asm__ volatile ("vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpaddsw %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu16 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(dst.i16[0] == INT16_MAX && dst.i16[1] == INT16_MIN && dst.i16[2] == INT16_MAX && dst.i16[3] == INT16_MIN,
        "VPADDSW exact saturation thresholds");

    a.u8[0] = UINT8_MAX; b.u8[0] = 1; a.u8[1] = UINT8_MAX - 1; b.u8[1] = 1; a.u8[2] = 0; b.u8[2] = 0;
    __asm__ volatile ("vmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvpaddusb %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(dst.u8[0] == UINT8_MAX && dst.u8[1] == UINT8_MAX && dst.u8[2] == 0, "VPADDUSB thresholds");

    a.u16[0] = UINT16_MAX; b.u16[0] = 1; a.u16[1] = UINT16_MAX - 1; b.u16[1] = 1; a.u16[2] = 0; b.u16[2] = 0;
    __asm__ volatile ("vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpaddusw %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu16 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(dst.u16[0] == UINT16_MAX && dst.u16[1] == UINT16_MAX && dst.u16[2] == 0, "VPADDUSW thresholds");

    uint64_t kmask = UINT64_C(0x8000000000000001);
    memset(&a, 1, sizeof(a)); memset(&b, 1, sizeof(b));
    __asm__ volatile ("kmovq %3,%%k1\n\tvmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvpaddsb %%zmm1,%%zmm0,%%zmm2%{%%k1%}%{z%}\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 64; i++) TEST_ASSERT(dst.i8[i] == ((i == 0 || i == 63) ? 2 : 0), "VPADDSB low/high mask lane %d", i);

    TEST_END();
}
