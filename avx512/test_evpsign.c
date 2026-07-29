/*
 * Test VPSIGNB/VPSIGNW/VPSIGND — SSSE3 instructions, no AVX-512 zmm form.
 * Implemented using ymm (256-bit) with VEX encoding (AVX2).
 * VPSIGNQ does not exist in any ISA extension.
 *
 * Compile: gcc -o test_evpsign avx512/test_evpsign.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

/* VPSIGN: dst[i] = a[i] if b[i]>0, -a[i] if b[i]<0, 0 if b[i]==0 */
static int8_t sign_i8(int8_t a, int8_t b) {
    if (b > 0) return a;
    if (b < 0) return (int8_t)(-a);
    return 0;
}
static int16_t sign_i16(int16_t a, int16_t b) {
    if (b > 0) return a;
    if (b < 0) return (int16_t)(-a);
    return 0;
}
static int32_t sign_i32(int32_t a, int32_t b) {
    if (b > 0) return a;
    if (b < 0) return -a;
    return 0;
}

int main(void) {
    if (!check_avx2()) {
        printf("AVX2 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPSIGNB/EVPSIGNW/EVPSIGND (ymm VEX/AVX2)");

    ymm_t a, b, dst;

    /* VPSIGNB ymm */
    for (int i = 0; i < 32; i++) {
        a.i8[i] = (int8_t)(i + 1);
        b.i8[i] = (int8_t)(i % 3 == 0 ? 1 : i % 3 == 1 ? -1 : 0);
    }
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpsignb %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.i8[i] == sign_i8(a.i8[i], b.i8[i]),
            "VPSIGNB lane %d: %d != %d", i, dst.i8[i], sign_i8(a.i8[i], b.i8[i]));

    /* VPSIGNW ymm */
    for (int i = 0; i < 16; i++) {
        a.i16[i] = (int16_t)((i + 1) * 100);
        b.i16[i] = (int16_t)(i % 3 == 0 ? 1 : i % 3 == 1 ? -1 : 0);
    }
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpsignw %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.i16[i] == sign_i16(a.i16[i], b.i16[i]),
            "VPSIGNW lane %d: %d != %d", i, dst.i16[i], sign_i16(a.i16[i], b.i16[i]));

    /* VPSIGND ymm */
    for (int i = 0; i < 8; i++) {
        a.i32[i] = (int)(i + 1) * 10000;
        b.i32[i] = (int)(i % 3 == 0 ? 1 : i % 3 == 1 ? -1 : 0);
    }
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpsignd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i32[i] == sign_i32(a.i32[i], b.i32[i]),
            "VPSIGND lane %d: %d != %d", i, dst.i32[i], sign_i32(a.i32[i], b.i32[i]));

    /* Boundary: all b=0 -> all dst=0 */
    for (int i = 0; i < 8; i++) { a.i32[i] = (int)(i + 1) * 7; b.i32[i] = 0; }
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpsignd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i32[i] == 0, "VPSIGND zero-b lane %d: %d", i, dst.i32[i]);

    /* Boundary: all b>0 -> dst=a */
    for (int i = 0; i < 8; i++) { a.i32[i] = -(i + 1) * 100; b.i32[i] = 1; }
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpsignd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.i32[i] == a.i32[i], "VPSIGND pos-b lane %d: %d", i, dst.i32[i]);

    TEST_END();
}
