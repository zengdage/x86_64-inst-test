/*
 * Test VPSUBSB/VPSUBSW/VPSUBUSB/VPSUBUSW with zmm registers (AVX-512BW).
 * Packed integer saturating subtraction (signed/unsigned byte/word).
 *
 * Compile: gcc -o test_evpsubs avx512/test_evpsubs.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

static int check_avx512bw(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}

static int8_t sat_sub_i8(int8_t a, int8_t b) {
    int r = (int)a - b;
    if (r > 127) return 127;
    if (r < -128) return -128;
    return (int8_t)r;
}
static int16_t sat_sub_i16(int16_t a, int16_t b) {
    int r = (int)a - b;
    if (r > 32767) return 32767;
    if (r < -32768) return -32768;
    return (int16_t)r;
}
static uint8_t sat_sub_u8(uint8_t a, uint8_t b) {
    return a > b ? a - b : 0;
}
static uint16_t sat_sub_u16(uint16_t a, uint16_t b) {
    return a > b ? a - b : 0;
}

int main(void) {
    if (!check_avx512bw()) {
        printf("AVX-512BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPSUBSB/EVPSUBSW/EVPSUBUSB/EVPSUBUSW (zmm)");

    zmm_t a, b, dst;

    /* VPSUBSB */
    for (int i = 0; i < 64; i++) { a.i8[i] = (int8_t)(i - 32); b.i8[i] = 100; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsubsb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.i8[i] == sat_sub_i8(a.i8[i], b.i8[i]),
            "VPSUBSB lane %d: %d != %d", i, dst.i8[i], sat_sub_i8(a.i8[i], b.i8[i]));

    /* VPSUBSW */
    for (int i = 0; i < 32; i++) { a.i16[i] = (int16_t)(i * 1000 - 16000); b.i16[i] = 30000; }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpsubsw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.i16[i] == sat_sub_i16(a.i16[i], b.i16[i]),
            "VPSUBSW lane %d: %d != %d", i, dst.i16[i], sat_sub_i16(a.i16[i], b.i16[i]));

    /* VPSUBUSB */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i * 3); b.u8[i] = 200; }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsubusb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == sat_sub_u8(a.u8[i], b.u8[i]),
            "VPSUBUSB lane %d: %u != %u", i, dst.u8[i], sat_sub_u8(a.u8[i], b.u8[i]));

    /* VPSUBUSW */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i * 2000); b.u16[i] = 60000; }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpsubusw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == sat_sub_u16(a.u16[i], b.u16[i]),
            "VPSUBUSW lane %d: %u != %u", i, dst.u16[i], sat_sub_u16(a.u16[i], b.u16[i]));

    /* Boundary: min - 1 saturates to -128 */
    memset(&a, 0x80, sizeof(a)); /* -128 */
    for (int i = 0; i < 64; i++) b.i8[i] = 1;
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpsubsb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    TEST_ASSERT(dst.i8[0] == -128, "VPSUBSB min sat: %d", dst.i8[0]);

    TEST_END();
}
