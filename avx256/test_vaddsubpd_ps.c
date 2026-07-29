/*
 * Test VADDSUBPD/VADDSUBPS
 * 256-bit alternating add/sub on pairs: odd lanes add, even lanes subtract
 * Compile: gcc -o test_vaddsubpd_ps avx256/test_vaddsubpd_ps.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "../common.h"

static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}

static void test_vaddsubpd256(void) {
    TEST_START("VADDSUBPD (256-bit)");
    /* ymm: [d0,d1,d2,d3], result[i]: i even -> sub, i odd -> add */
    double a[4] = {10.0, 20.0, 30.0, 40.0};
    double b[4] = {1.0,  2.0,  3.0,  4.0};
    double r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vaddsubpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    /* even indices (0,2): sub; odd indices (1,3): add */
    TEST_ASSERT(r[0] == 9.0,  "vaddsubpd r[0] expected 9.0 got %f", r[0]);
    TEST_ASSERT(r[1] == 22.0, "vaddsubpd r[1] expected 22.0 got %f", r[1]);
    TEST_ASSERT(r[2] == 27.0, "vaddsubpd r[2] expected 27.0 got %f", r[2]);
    TEST_ASSERT(r[3] == 44.0, "vaddsubpd r[3] expected 44.0 got %f", r[3]);

    /* mem operand form */
    double r2[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vaddsubpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 9.0,  "vaddsubpd mem r[0] expected 9.0 got %f", r2[0]);
    TEST_ASSERT(r2[1] == 22.0, "vaddsubpd mem r[1] expected 22.0 got %f", r2[1]);
}

static void test_vaddsubps256(void) {
    TEST_START("VADDSUBPS (256-bit)");
    float a[8] = {10,20,30,40,50,60,70,80};
    float b[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vaddsubps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    /* even: sub, odd: add */
    TEST_ASSERT(r[0] == 9.0f,  "vaddsubps r[0] expected 9 got %f", r[0]);
    TEST_ASSERT(r[1] == 22.0f, "vaddsubps r[1] expected 22 got %f", r[1]);
    TEST_ASSERT(r[2] == 27.0f, "vaddsubps r[2] expected 27 got %f", r[2]);
    TEST_ASSERT(r[3] == 44.0f, "vaddsubps r[3] expected 44 got %f", r[3]);
    TEST_ASSERT(r[4] == 45.0f, "vaddsubps r[4] expected 45 got %f", r[4]);
    TEST_ASSERT(r[5] == 66.0f, "vaddsubps r[5] expected 66 got %f", r[5]);
    TEST_ASSERT(r[6] == 63.0f, "vaddsubps r[6] expected 63 got %f", r[6]);
    TEST_ASSERT(r[7] == 88.0f, "vaddsubps r[7] expected 88 got %f", r[7]);

    /* mem operand */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vaddsubps %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 9.0f, "vaddsubps mem r[0] expected 9 got %f", r2[0]);
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vaddsubpd256();
    test_vaddsubps256();
    TEST_END();
}
