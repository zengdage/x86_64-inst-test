/*
 * Test VMOVNTDQ/VMOVNTDQA/VMOVNTPD/VMOVNTPS with zmm registers (AVX-512F).
 * Non-temporal (streaming) store/load instructions.
 *
 * Compile: gcc -o test_evmovnt avx512/test_evmovnt.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    return (ebx >> 16) & 1;
}

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVMOVNT (VMOVNTDQ/VMOVNTDQA/VMOVNTPD/VMOVNTPS)");

    zmm_t src, dst;

    /* VMOVNTDQ: non-temporal store zmm -> mem */
    for (int i = 0; i < 8; i++) src.u64[i] = 0xA5A5A5A5A5A5A5A5ULL ^ (uint64_t)i;
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovdqa64 %1, %%zmm0\n\t"
        "vmovntdq %%zmm0, %0"
        : "=m"(dst) : "m"(src) : "zmm0"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQ store failed");

    /* VMOVNTDQA: non-temporal load mem -> zmm */
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovntdqa %1, %%zmm1\n\t"
        "vmovdqa64 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm1"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQA load failed");

    /* VMOVNTPD: non-temporal store packed doubles */
    for (int i = 0; i < 8; i++) src.f64[i] = (double)(i + 1) * 1.5;
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovapd %1, %%zmm2\n\t"
        "vmovntpd %%zmm2, %0"
        : "=m"(dst) : "m"(src) : "zmm2"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTPD store failed");

    /* VMOVNTPS: non-temporal store packed floats */
    for (int i = 0; i < 16; i++) src.f32[i] = (float)(i + 1) * 0.5f;
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovaps %1, %%zmm3\n\t"
        "vmovntps %%zmm3, %0"
        : "=m"(dst) : "m"(src) : "zmm3"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTPS store failed");

    /* Boundary: all zeros */
    memset(&src, 0, sizeof(src));
    memset(&dst, 0xFF, sizeof(dst));
    __asm__ volatile (
        "vmovdqa64 %1, %%zmm4\n\t"
        "vmovntdq %%zmm4, %0"
        : "=m"(dst) : "m"(src) : "zmm4"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQ all-zeros failed");

    /* Boundary: all ones */
    memset(&src, 0xFF, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovdqa64 %1, %%zmm5\n\t"
        "vmovntdq %%zmm5, %0"
        : "=m"(dst) : "m"(src) : "zmm5"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQ all-ones failed");

    TEST_END();
}
