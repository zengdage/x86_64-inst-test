/*
 * Test VMOVNTDQ/VMOVNTPD/VMOVNTPS (256-bit non-temporal stores) + VMOVNTDQA (load)
 * Non-temporal (streaming) memory operations bypassing cache
 * Compile: gcc -o test_vmovnt256 avx256/test_vmovnt256.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common.h"

static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(7),"c"(0));
    return (ebx >> 5) & 1;
}

static void test_vmovntdq(void) {
    TEST_START("VMOVNTDQ (256-bit non-temporal integer store)");
    /* must be 32-byte aligned */
    int32_t src[8] = {1,2,3,4,5,6,7,8};
    int32_t __attribute__((aligned(32))) dst[8] = {0};
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vmovntdq %%ymm0, %0\n\t"
        "sfence\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "ymm0","memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntdq dst[%d]=%d", i, dst[i]);
}

static void test_vmovntpd(void) {
    TEST_START("VMOVNTPD (256-bit non-temporal double store)");
    double src[4] = {1.1, 2.2, 3.3, 4.4};
    double __attribute__((aligned(32))) dst[4] = {0};
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovntpd %%ymm0, %0\n\t"
        "sfence\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "ymm0","memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntpd dst[%d]=%f", i, dst[i]);
}

static void test_vmovntps(void) {
    TEST_START("VMOVNTPS (256-bit non-temporal float store)");
    float src[8] = {1,2,3,4,5,6,7,8};
    float __attribute__((aligned(32))) dst[8] = {0};
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovntps %%ymm0, %0\n\t"
        "sfence\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "ymm0","memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntps dst[%d]=%f", i, dst[i]);
}

static void test_vmovntdqa(void) {
    TEST_START("VMOVNTDQA (128-bit non-temporal integer load)");
    /* vmovntdqa is 128-bit xmm only in AVX/AVX2 (256-bit form needs AVX-512) */
    int32_t __attribute__((aligned(16))) src[4] = {10,20,30,40};
    int32_t dst[4] = {0};
    __asm__ volatile(
        "vmovntdqa %1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "xmm0","memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntdqa dst[%d]=%d", i, dst[i]);
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vmovntdq();
    test_vmovntpd();
    test_vmovntps();
    test_vmovntdqa();
    TEST_END();
}
