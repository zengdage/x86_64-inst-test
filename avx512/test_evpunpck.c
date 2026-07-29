/*
 * Test VPUNPCKLBW/VPUNPCKLWD/VPUNPCKLDQ/VPUNPCKLQDQ/
 *      VPUNPCKHBW/VPUNPCKHWD/VPUNPCKHDQ/VPUNPCKHQDQ with zmm (AVX-512BW/F).
 * Packed unpack low/high bytes/words/dwords/qwords.
 *
 * Compile: gcc -o test_evpunpck avx512/test_evpunpck.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512F/BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPUNPCK* (VPUNPCKLBW/WD/DQ/QDQ + VPUNPCKHBW/WD/DQ/QDQ zmm)");

    zmm_t a, b, dst;

    /* VPUNPCKLBW: interleave low 8 bytes of each 16-byte lane from a and b */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i + 1); b.u8[i] = (uint8_t)(i + 100); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpunpcklbw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 8; j++) {
            TEST_ASSERT(dst.u8[lane*16 + j*2]   == a.u8[lane*16 + j],
                "VPUNPCKLBW lane%d a%d: %u", lane, j, dst.u8[lane*16+j*2]);
            TEST_ASSERT(dst.u8[lane*16 + j*2+1] == b.u8[lane*16 + j],
                "VPUNPCKLBW lane%d b%d: %u", lane, j, dst.u8[lane*16+j*2+1]);
        }
    }

    /* VPUNPCKHBW: interleave high 8 bytes of each 16-byte lane */
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpunpckhbw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 8; j++) {
            TEST_ASSERT(dst.u8[lane*16 + j*2]   == a.u8[lane*16 + 8 + j],
                "VPUNPCKHBW lane%d a%d: %u", lane, j, dst.u8[lane*16+j*2]);
            TEST_ASSERT(dst.u8[lane*16 + j*2+1] == b.u8[lane*16 + 8 + j],
                "VPUNPCKHBW lane%d b%d: %u", lane, j, dst.u8[lane*16+j*2+1]);
        }
    }

    /* VPUNPCKLWD: interleave low 4 words of each 16-byte lane */
    for (int i = 0; i < 32; i++) { a.u16[i] = (uint16_t)(i + 1); b.u16[i] = (uint16_t)(i + 200); }
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpunpcklwd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 4; j++) {
            TEST_ASSERT(dst.u16[lane*8 + j*2]   == a.u16[lane*8 + j],
                "VPUNPCKLWD lane%d a%d: %u", lane, j, dst.u16[lane*8+j*2]);
            TEST_ASSERT(dst.u16[lane*8 + j*2+1] == b.u16[lane*8 + j],
                "VPUNPCKLWD lane%d b%d: %u", lane, j, dst.u16[lane*8+j*2+1]);
        }
    }

    /* VPUNPCKHWD */
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vmovdqu16 %2, %%zmm1\n\t"
        "vpunpckhwd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu16 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 4; j++) {
            TEST_ASSERT(dst.u16[lane*8 + j*2]   == a.u16[lane*8 + 4 + j],
                "VPUNPCKHWD lane%d a%d: %u", lane, j, dst.u16[lane*8+j*2]);
            TEST_ASSERT(dst.u16[lane*8 + j*2+1] == b.u16[lane*8 + 4 + j],
                "VPUNPCKHWD lane%d b%d: %u", lane, j, dst.u16[lane*8+j*2+1]);
        }
    }

    /* VPUNPCKLDQ: interleave low 2 dwords of each 16-byte lane */
    for (int i = 0; i < 16; i++) { a.u32[i] = (uint32_t)(i + 1); b.u32[i] = (uint32_t)(i + 100); }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpunpckldq %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        TEST_ASSERT(dst.u32[lane*4+0] == a.u32[lane*4+0], "VPUNPCKLDQ lane%d a0: %u", lane, dst.u32[lane*4+0]);
        TEST_ASSERT(dst.u32[lane*4+1] == b.u32[lane*4+0], "VPUNPCKLDQ lane%d b0: %u", lane, dst.u32[lane*4+1]);
        TEST_ASSERT(dst.u32[lane*4+2] == a.u32[lane*4+1], "VPUNPCKLDQ lane%d a1: %u", lane, dst.u32[lane*4+2]);
        TEST_ASSERT(dst.u32[lane*4+3] == b.u32[lane*4+1], "VPUNPCKLDQ lane%d b1: %u", lane, dst.u32[lane*4+3]);
    }

    /* VPUNPCKHDQ */
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpunpckhdq %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        TEST_ASSERT(dst.u32[lane*4+0] == a.u32[lane*4+2], "VPUNPCKHDQ lane%d a2: %u", lane, dst.u32[lane*4+0]);
        TEST_ASSERT(dst.u32[lane*4+1] == b.u32[lane*4+2], "VPUNPCKHDQ lane%d b2: %u", lane, dst.u32[lane*4+1]);
        TEST_ASSERT(dst.u32[lane*4+2] == a.u32[lane*4+3], "VPUNPCKHDQ lane%d a3: %u", lane, dst.u32[lane*4+2]);
        TEST_ASSERT(dst.u32[lane*4+3] == b.u32[lane*4+3], "VPUNPCKHDQ lane%d b3: %u", lane, dst.u32[lane*4+3]);
    }

    /* VPUNPCKLQDQ: interleave low qword of each 16-byte lane */
    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)(i + 1) * 100; b.u64[i] = (uint64_t)(i + 1) * 200; }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpunpcklqdq %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        TEST_ASSERT(dst.u64[lane*2+0] == a.u64[lane*2+0], "VPUNPCKLQDQ lane%d a: %llu", lane, (unsigned long long)dst.u64[lane*2]);
        TEST_ASSERT(dst.u64[lane*2+1] == b.u64[lane*2+0], "VPUNPCKLQDQ lane%d b: %llu", lane, (unsigned long long)dst.u64[lane*2+1]);
    }

    /* VPUNPCKHQDQ */
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vpunpckhqdq %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        TEST_ASSERT(dst.u64[lane*2+0] == a.u64[lane*2+1], "VPUNPCKHQDQ lane%d a: %llu", lane, (unsigned long long)dst.u64[lane*2]);
        TEST_ASSERT(dst.u64[lane*2+1] == b.u64[lane*2+1], "VPUNPCKHQDQ lane%d b: %llu", lane, (unsigned long long)dst.u64[lane*2+1]);
    }

    TEST_END();
}
