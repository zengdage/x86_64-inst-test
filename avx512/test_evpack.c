/*
 * Test VPACKSSWB/VPACKSSDW/VPACKUSWB/VPACKUSDW with zmm registers (AVX-512BW/F).
 * Packed pack/saturate signed/unsigned word/dword to byte/word.
 *
 * Compile: gcc -o test_evpack avx512/test_evpack.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

static int8_t sat_i16_to_i8(int16_t v) { return v > 127 ? 127 : v < -128 ? -128 : (int8_t)v; }
static int16_t sat_i32_to_i16(int32_t v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v; }
static uint8_t sat_i16_to_u8(int16_t v) { return v > 255 ? 255 : v < 0 ? 0 : (uint8_t)v; }
static uint16_t sat_i32_to_u16(int32_t v) { return v > 65535 ? 65535 : v < 0 ? 0 : (uint16_t)v; }

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512F/BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPACKSSWB/EVPACKSSDW/EVPACKUSWB/EVPACKUSDW (zmm)");

    zmm_t a, b, dst;

    /*
     * VPACKSSWB zmm: pack 32 signed words from a and 32 from b into 64 signed bytes.
     * Within each 16-byte lane: 8 bytes from a, then 8 bytes from b.
     */
    for (int i = 0; i < 32; i++) a.i16[i] = (int16_t)(i * 50 - 400);
    for (int i = 0; i < 32; i++) b.i16[i] = (int16_t)(i * 30 - 200);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpacksswb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    /* Each 16-byte output lane: 8 bytes from a-lane, 8 bytes from b-lane */
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 8; j++)
            TEST_ASSERT(dst.i8[lane*16+j] == sat_i16_to_i8(a.i16[lane*8+j]),
                "VPACKSSWB a lane%d j%d: %d", lane, j, dst.i8[lane*16+j]);
        for (int j = 0; j < 8; j++)
            TEST_ASSERT(dst.i8[lane*16+8+j] == sat_i16_to_i8(b.i16[lane*8+j]),
                "VPACKSSWB b lane%d j%d: %d", lane, j, dst.i8[lane*16+8+j]);
    }

    /* VPACKSSDW zmm: pack 16 signed dwords from a and 16 from b into 32 signed words */
    for (int i = 0; i < 16; i++) a.i32[i] = (int)(i * 5000 - 40000);
    for (int i = 0; i < 16; i++) b.i32[i] = (int)(i * 3000 - 20000);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpackssdw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 4; j++)
            TEST_ASSERT(dst.i16[lane*8+j] == sat_i32_to_i16(a.i32[lane*4+j]),
                "VPACKSSDW a lane%d j%d: %d", lane, j, dst.i16[lane*8+j]);
        for (int j = 0; j < 4; j++)
            TEST_ASSERT(dst.i16[lane*8+4+j] == sat_i32_to_i16(b.i32[lane*4+j]),
                "VPACKSSDW b lane%d j%d: %d", lane, j, dst.i16[lane*8+4+j]);
    }

    /* VPACKUSWB zmm: pack 32 signed words into 64 unsigned bytes (clamp 0..255) */
    for (int i = 0; i < 32; i++) a.i16[i] = (int16_t)(i * 20 - 100);
    for (int i = 0; i < 32; i++) b.i16[i] = (int16_t)(i * 15);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpackuswb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 8; j++)
            TEST_ASSERT(dst.u8[lane*16+j] == sat_i16_to_u8(a.i16[lane*8+j]),
                "VPACKUSWB a lane%d j%d: %u", lane, j, dst.u8[lane*16+j]);
        for (int j = 0; j < 8; j++)
            TEST_ASSERT(dst.u8[lane*16+8+j] == sat_i16_to_u8(b.i16[lane*8+j]),
                "VPACKUSWB b lane%d j%d: %u", lane, j, dst.u8[lane*16+8+j]);
    }

    /* VPACKUSDW zmm: pack 16 unsigned dwords into 32 unsigned words (clamp 0..65535) */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i * 10000);
    for (int i = 0; i < 16; i++) b.u32[i] = (uint32_t)(i * 5000 + 70000);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpackusdw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 4; j++)
            TEST_ASSERT(dst.u16[lane*8+j] == sat_i32_to_u16(a.i32[lane*4+j]),
                "VPACKUSDW a lane%d j%d: %u", lane, j, dst.u16[lane*8+j]);
        for (int j = 0; j < 4; j++)
            TEST_ASSERT(dst.u16[lane*8+4+j] == sat_i32_to_u16(b.i32[lane*4+j]),
                "VPACKUSDW b lane%d j%d: %u", lane, j, dst.u16[lane*8+4+j]);
    }

    const int16_t i8_edges[8] = {-129, -128, -127, 126, 127, 128, INT16_MIN, INT16_MAX};
    for (int i = 0; i < 32; i++) a.i16[i] = b.i16[i] = i8_edges[i & 7];
    __asm__ volatile ("vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpacksswb %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.i8[i] == sat_i16_to_i8(i8_edges[i]), "VPACKSSWB exact threshold %d", i);

    const int32_t i16_edges[4] = {-32769, -32768, 32767, 32768};
    for (int i = 0; i < 16; i++) a.i32[i] = b.i32[i] = i16_edges[i & 3];
    __asm__ volatile ("vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvpackssdw %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu16 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 4; i++) TEST_ASSERT(dst.i16[i] == sat_i32_to_i16(i16_edges[i]), "VPACKSSDW exact threshold %d", i);

    const int16_t u8_edges[8] = {-1, 0, 1, 254, 255, 256, INT16_MIN, INT16_MAX};
    for (int i = 0; i < 32; i++) a.i16[i] = b.i16[i] = u8_edges[i & 7];
    __asm__ volatile ("vmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpackuswb %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u8[i] == sat_i16_to_u8(u8_edges[i]), "VPACKUSWB exact threshold %d", i);

    const int32_t u16_edges[4] = {-1, 0, 65535, 65536};
    for (int i = 0; i < 16; i++) a.i32[i] = b.i32[i] = u16_edges[i & 3];
    __asm__ volatile ("vmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvpackusdw %%zmm1,%%zmm0,%%zmm2\n\tvmovdqu16 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 4; i++) TEST_ASSERT(dst.u16[i] == sat_i32_to_u16(u16_edges[i]), "VPACKUSDW exact threshold %d", i);

    uint64_t kmask = UINT64_C(0x8000000000000001);
    for (int i = 0; i < 32; i++) a.i16[i] = b.i16[i] = 42;
    __asm__ volatile ("kmovq %3,%%k1\n\tvmovdqu16 %1,%%zmm0\n\tvmovdqu16 %2,%%zmm1\n\tvpacksswb %%zmm1,%%zmm0,%%zmm2%{%%k1%}%{z%}\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 64; i++) TEST_ASSERT(dst.i8[i] == ((i == 0 || i == 63) ? 42 : 0), "VPACKSSWB low/high zero mask lane %d", i);

    TEST_END();
}
