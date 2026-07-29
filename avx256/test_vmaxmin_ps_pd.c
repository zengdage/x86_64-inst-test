/*
 * Test VMAXPD/VMAXPS/VMINPD/VMINPS (256-bit packed) + VMAXSD/VMAXSS/VMINSD/VMINSS (scalar)
 * Packed and scalar max/min operations on float and double
 * Compile: gcc -o test_vmaxmin_ps_pd avx256/test_vmaxmin_ps_pd.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include "../common.h"

static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}

static void test_vmaxpd(void) {
    TEST_START("VMAXPD (256-bit)");
    double a[4] = {1.0, 5.0, -3.0, 8.0};
    double b[4] = {2.0, 4.0,  3.0, 7.0};
    double r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vmaxpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 2.0, "vmaxpd r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 5.0, "vmaxpd r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == 3.0, "vmaxpd r[2]=%f", r[2]);
    TEST_ASSERT(r[3] == 8.0, "vmaxpd r[3]=%f", r[3]);
    /* mem form */
    double r2[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmaxpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 2.0, "vmaxpd mem r[0]=%f", r2[0]);
}

static void test_vmaxps(void) {
    TEST_START("VMAXPS (256-bit)");
    float a[8] = {1,5,-3,8,10,-2,0,7};
    float b[8] = {2,4, 3,7, 9, 3,1,6};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vmaxps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 2.0f,  "vmaxps r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 5.0f,  "vmaxps r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == 3.0f,  "vmaxps r[2]=%f", r[2]);
    TEST_ASSERT(r[4] == 10.0f, "vmaxps r[4]=%f", r[4]);
    /* mem form */
    float r2[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmaxps %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 2.0f, "vmaxps mem r[0]=%f", r2[0]);
}

static void test_vminpd(void) {
    TEST_START("VMINPD (256-bit)");
    double a[4] = {1.0, 5.0, -3.0, 8.0};
    double b[4] = {2.0, 4.0,  3.0, 7.0};
    double r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vminpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 1.0,  "vminpd r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 4.0,  "vminpd r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == -3.0, "vminpd r[2]=%f", r[2]);
    TEST_ASSERT(r[3] == 7.0,  "vminpd r[3]=%f", r[3]);
    /* mem form */
    double r2[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vminpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 1.0, "vminpd mem r[0]=%f", r2[0]);
}

static void test_vminps(void) {
    TEST_START("VMINPS (256-bit)");
    float a[8] = {1,5,-3,8,10,-2,0,7};
    float b[8] = {2,4, 3,7, 9, 3,1,6};
    float r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vminps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 1.0f,  "vminps r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 4.0f,  "vminps r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == -3.0f, "vminps r[2]=%f", r[2]);
    TEST_ASSERT(r[4] == 9.0f,  "vminps r[4]=%f", r[4]);
}

static void test_vmaxsd(void) {
    TEST_START("VMAXSD (scalar double)");
    double a = 3.0, b = 5.0;
    double r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmaxsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 5.0, "vmaxsd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmaxsd %2, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 5.0, "vmaxsd mem r=%f", r);
}

static void test_vmaxss(void) {
    TEST_START("VMAXSS (scalar float)");
    float a = 3.0f, b = 5.0f;
    float r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmaxss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 5.0f, "vmaxss r=%f", r);
}

static void test_vminsd(void) {
    TEST_START("VMINSD (scalar double)");
    double a = 3.0, b = 5.0;
    double r;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vminsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 3.0, "vminsd r=%f", r);
    /* mem form */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vminsd %2, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm2"
    );
    TEST_ASSERT(r == 3.0, "vminsd mem r=%f", r);
}

static void test_vminss(void) {
    TEST_START("VMINSS (scalar float)");
    float a = 3.0f, b = 5.0f;
    float r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vminss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r == 3.0f, "vminss r=%f", r);
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vmaxpd();
    test_vmaxps();
    test_vminpd();
    test_vminps();
    test_vmaxsd();
    test_vmaxss();
    test_vminsd();
    test_vminss();
    TEST_END();
}
