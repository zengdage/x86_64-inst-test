/*
 * Test VPHADDD/VPHADDW/VPHADDSW/VPHSUBD/VPHSUBW/VPHSUBSW — SSSE3 instructions,
 * no AVX-512 zmm form. Implemented using ymm (256-bit) with VEX encoding (AVX2).
 *
 * Compile: gcc -o test_evphadd_phsub avx512/test_evphadd_phsub.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 5) & 1;
}

int main(void) {
    if (!check_avx2()) {
        printf("AVX2 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPHADDD/EVPHADDW/EVPHADDSW/EVPHSUBD/EVPHSUBW/EVPHSUBSW (ymm VEX/AVX2)");

    ymm_t a, b, dst;

    /* VPHADDD ymm: horizontal add adjacent dwords */
    for (int i = 0; i < 8; i++) a.i32[i] = (int)(i + 1);
    for (int i = 0; i < 8; i++) b.i32[i] = (int)(i + 10);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vphaddd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    /* Lower 128: a[0]+a[1], a[2]+a[3], b[0]+b[1], b[2]+b[3] */
    TEST_ASSERT(dst.i32[0] == a.i32[0]+a.i32[1], "VPHADDD lo[0]: %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == a.i32[2]+a.i32[3], "VPHADDD lo[1]: %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == b.i32[0]+b.i32[1], "VPHADDD lo[2]: %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == b.i32[2]+b.i32[3], "VPHADDD lo[3]: %d", dst.i32[3]);
    /* Upper 128: a[4]+a[5], a[6]+a[7], b[4]+b[5], b[6]+b[7] */
    TEST_ASSERT(dst.i32[4] == a.i32[4]+a.i32[5], "VPHADDD hi[4]: %d", dst.i32[4]);
    TEST_ASSERT(dst.i32[5] == a.i32[6]+a.i32[7], "VPHADDD hi[5]: %d", dst.i32[5]);
    TEST_ASSERT(dst.i32[6] == b.i32[4]+b.i32[5], "VPHADDD hi[6]: %d", dst.i32[6]);
    TEST_ASSERT(dst.i32[7] == b.i32[6]+b.i32[7], "VPHADDD hi[7]: %d", dst.i32[7]);

    /* VPHADDW ymm: horizontal add adjacent words */
    for (int i = 0; i < 16; i++) a.i16[i] = (int16_t)(i + 1);
    for (int i = 0; i < 16; i++) b.i16[i] = (int16_t)(i + 100);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vphaddw %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    /* Lower 128: 4 pairs from a, 4 pairs from b */
    for (int j = 0; j < 4; j++)
        TEST_ASSERT(dst.i16[j] == (int16_t)(a.i16[j*2]+a.i16[j*2+1]),
            "VPHADDW lo a[%d]: %d", j, dst.i16[j]);
    for (int j = 0; j < 4; j++)
        TEST_ASSERT(dst.i16[4+j] == (int16_t)(b.i16[j*2]+b.i16[j*2+1]),
            "VPHADDW lo b[%d]: %d", j, dst.i16[4+j]);

    /* VPHADDSW ymm: horizontal add adjacent words with saturation */
    for (int i = 0; i < 16; i++) a.i16[i] = (int16_t)(i < 8 ? 30000 : -30000);
    for (int i = 0; i < 16; i++) b.i16[i] = (int16_t)(i < 8 ? 20000 : -20000);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vphaddsw %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(dst.i16[0] == 32767, "VPHADDSW sat pos: %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[4] == 32767, "VPHADDSW sat pos b: %d", dst.i16[4]);

    /* VPHSUBD ymm: horizontal subtract adjacent dwords */
    for (int i = 0; i < 8; i++) a.i32[i] = (int)(i * 10 + 5);
    for (int i = 0; i < 8; i++) b.i32[i] = (int)(i * 10 + 3);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vphsubd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(dst.i32[0] == a.i32[0]-a.i32[1], "VPHSUBD lo[0]: %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[2] == b.i32[0]-b.i32[1], "VPHSUBD lo[2]: %d", dst.i32[2]);

    /* VPHSUBW ymm: horizontal subtract adjacent words */
    for (int i = 0; i < 16; i++) a.i16[i] = (int16_t)(i * 5 + 10);
    for (int i = 0; i < 16; i++) b.i16[i] = (int16_t)(i * 3 + 5);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vphsubw %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    for (int j = 0; j < 4; j++)
        TEST_ASSERT(dst.i16[j] == (int16_t)(a.i16[j*2]-a.i16[j*2+1]),
            "VPHSUBW lo a[%d]: %d", j, dst.i16[j]);

    /* VPHSUBSW ymm: horizontal subtract with saturation */
    for (int i = 0; i < 16; i++) a.i16[i] = (int16_t)((i % 2 == 0) ? -30000 : 30000);
    for (int i = 0; i < 16; i++) b.i16[i] = (int16_t)(i < 8 ? 20000 : -20000);
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vphsubsw %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(dst.i16[0] == -32768, "VPHSUBSW sat neg: %d", dst.i16[0]);

    TEST_END();
}
