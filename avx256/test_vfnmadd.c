/*
 * Test VFNMADD132PS/VFNMADD213PS/VFNMADD231PS/PD + VFNMADDSD/SS
 * FMA3 negated multiply-add: -(a*b) + c
 * Compile: gcc -o test_vfnmadd avx256/test_vfnmadd.c -O0 -mavx2 -mfma
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "../common.h"

static int check_fma(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 12) & 1;
}

/* VFNMADD132PS: dst = -(dst * src2) + src1  (132: dst*src2, add src1) */
static void test_vfnmadd132ps(void) {
    TEST_START("VFNMADD132PS (256-bit)");
    float a[8] = {2,2,2,2,2,2,2,2}; /* dst */
    float b[8] = {3,3,3,3,3,3,3,3}; /* src1 (addend) */
    float c[8] = {4,4,4,4,4,4,4,4}; /* src2 */
    float r[8];
    /* r = -(a*c) + b = -(2*4)+3 = -5 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"  /* dst=a */
        "vmovups %2, %%ymm1\n\t"  /* src1=b */
        "vmovups %3, %%ymm2\n\t"  /* src2=c */
        "vfnmadd132ps %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == -5.0f, "vfnmadd132ps r[%d]=%f", i, r[i]);
    /* mem form: src2 from memory */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vfnmadd132ps %3, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r2[0] == -5.0f, "vfnmadd132ps mem r[0]=%f", r2[0]);
}

/* VFNMADD213PS: dst = -(src1*dst) + src2  (213: src1*dst, add src2) */
static void test_vfnmadd213ps(void) {
    TEST_START("VFNMADD213PS (256-bit)");
    float a[8] = {2,2,2,2,2,2,2,2}; /* dst */
    float b[8] = {3,3,3,3,3,3,3,3}; /* src1 */
    float c[8] = {5,5,5,5,5,5,5,5}; /* src2 (addend) */
    float r[8];
    /* r = -(b*a) + c = -(3*2)+5 = -1 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vmovups %3, %%ymm2\n\t"
        "vfnmadd213ps %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == -1.0f, "vfnmadd213ps r[%d]=%f", i, r[i]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vfnmadd213ps %3, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r2[0] == -1.0f, "vfnmadd213ps mem r[0]=%f", r2[0]);
}

/* VFNMADD231PS: dst = -(src1*src2) + dst  (231: src1*src2, add dst) */
static void test_vfnmadd231ps(void) {
    TEST_START("VFNMADD231PS (256-bit)");
    float a[8] = {10,10,10,10,10,10,10,10}; /* dst (addend) */
    float b[8] = {3,3,3,3,3,3,3,3};         /* src1 */
    float c[8] = {4,4,4,4,4,4,4,4};         /* src2 */
    float r[8];
    /* r = -(b*c) + a = -(3*4)+10 = -2 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vmovups %3, %%ymm2\n\t"
        "vfnmadd231ps %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == -2.0f, "vfnmadd231ps r[%d]=%f", i, r[i]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vfnmadd231ps %3, %%ymm1, %%ymm0\n\t"
        "vmovups %%ymm0, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(r2[0] == -2.0f, "vfnmadd231ps mem r[0]=%f", r2[0]);
}

/* VFNMADD132PD/213PD/231PD (256-bit double) */
static void test_vfnmadd_pd(void) {
    TEST_START("VFNMADD132PD/213PD/231PD (256-bit double)");
    double a[4] = {2,2,2,2};
    double b[4] = {3,3,3,3};
    double c[4] = {4,4,4,4};
    double r[4];
    /* 132: -(a*c)+b = -(2*4)+3 = -5 */
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmovupd %3, %%ymm2\n\t"
        "vfnmadd132pd %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == -5.0, "vfnmadd132pd r[%d]=%f", i, r[i]);
    /* 213: -(b*a)+c = -(3*2)+4 = -2 */
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmovupd %3, %%ymm2\n\t"
        "vfnmadd213pd %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == -2.0, "vfnmadd213pd r[%d]=%f", i, r[i]);
    /* 231: -(b*c)+a = -(3*4)+2 = -10 */
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmovupd %3, %%ymm2\n\t"
        "vfnmadd231pd %%ymm2, %%ymm1, %%ymm0\n\t"
        "vmovupd %%ymm0, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]), "m"(c[0]) : "ymm0","ymm1","ymm2"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == -10.0, "vfnmadd231pd r[%d]=%f", i, r[i]);
}

/* VFNMADD132SD/213SD/231SD (scalar double) */
static void test_vfnmadd_sd(void) {
    TEST_START("VFNMADD132SD/213SD/231SD (scalar double)");
    double a = 2.0, b = 3.0, c = 4.0, r;
    /* 132: -(a*c)+b = -5 */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %3, %%xmm2\n\t"
        "vfnmadd132sd %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -5.0, "vfnmadd132sd r=%f", r);
    /* 213: -(b*a)+c = -2 */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %3, %%xmm2\n\t"
        "vfnmadd213sd %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -2.0, "vfnmadd213sd r=%f", r);
    /* 231: -(b*c)+a = -10 */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %3, %%xmm2\n\t"
        "vfnmadd231sd %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -10.0, "vfnmadd231sd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vfnmadd132sd %3, %%xmm1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1"
    );
    TEST_ASSERT(r == -5.0, "vfnmadd132sd mem r=%f", r);
}

/* VFNMADD132SS/213SS/231SS (scalar float) */
static void test_vfnmadd_ss(void) {
    TEST_START("VFNMADD132SS/213SS/231SS (scalar float)");
    float a = 2.0f, b = 3.0f, c = 4.0f, r;
    /* 132: -(a*c)+b = -5 */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmovss %3, %%xmm2\n\t"
        "vfnmadd132ss %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovss %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -5.0f, "vfnmadd132ss r=%f", r);
    /* 213: -(b*a)+c = -2 */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmovss %3, %%xmm2\n\t"
        "vfnmadd213ss %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovss %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -2.0f, "vfnmadd213ss r=%f", r);
    /* 231: -(b*c)+a = -10 */
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmovss %3, %%xmm2\n\t"
        "vfnmadd231ss %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovss %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b), "m"(c) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == -10.0f, "vfnmadd231ss r=%f", r);
}

int main(void) {
    if (!check_fma()) { printf("FMA not supported\n"); return 1; }
    test_vfnmadd132ps();
    test_vfnmadd213ps();
    test_vfnmadd231ps();
    test_vfnmadd_pd();
    test_vfnmadd_sd();
    test_vfnmadd_ss();
    TEST_END();
}
