/*
 * Test VMOVSD/VMOVSS (scalar load/store/reg-reg) + VMOVHPD/VMOVHPS/VMOVLPD/VMOVLPS
 * Scalar and high/low packed float/double move instructions
 * Compile: gcc -o test_vmovscalar avx256/test_vmovscalar.c -O0 -mavx2
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

static void test_vmovsd(void) {
    TEST_START("VMOVSD (load/store/reg-reg)");
    double a = 3.14, r;
    /* load from mem */
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r == 3.14, "vmovsd load/store r=%f", r);
    /* reg-reg (3-operand: merges upper bits from src1) */
    double b = 2.71, r2;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vmovsd %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovsd %%xmm2, %0\n\t"
        : "=m"(r2) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r2 == 2.71, "vmovsd reg-reg r=%f", r2);
}

static void test_vmovss(void) {
    TEST_START("VMOVSS (load/store/reg-reg)");
    float a = 1.5f, r;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %%xmm0, %0\n\t"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r == 1.5f, "vmovss load/store r=%f", r);
    float b = 2.5f, r2;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vmovss %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovss %%xmm2, %0\n\t"
        : "=m"(r2) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r2 == 2.5f, "vmovss reg-reg r=%f", r2);
}

static void test_vmovhpd(void) {
    TEST_START("VMOVHPD (load high 64-bit double from/to mem)");
    /* load: mem -> xmm high quadword */
    double lo = 1.0, hi = 2.0;
    double r[2];
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"       /* xmm0[63:0] = lo */
        "vmovhpd %2, %%xmm0, %%xmm1\n\t" /* xmm1[127:64]=hi, xmm1[63:0]=lo */
        "vmovupd %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(lo), "m"(hi) : "xmm0","xmm1"
    );
    TEST_ASSERT(r[0] == 1.0, "vmovhpd load r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 2.0, "vmovhpd load r[1]=%f", r[1]);
    /* store: xmm high quadword -> mem */
    double out;
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovhpd %%xmm0, %0\n\t"
        : "=m"(out) : "m"(r[0]) : "xmm0"
    );
    TEST_ASSERT(out == 2.0, "vmovhpd store out=%f", out);
}

static void test_vmovhps(void) {
    TEST_START("VMOVHPS (load high 64-bit from/to mem as two floats)");
    float mem[2] = {3.0f, 4.0f};
    float xmm_init[4] = {1.0f, 2.0f, 0.0f, 0.0f};
    float r[4];
    __asm__ volatile(
        "vmovups %1, %%xmm0\n\t"
        "vmovhps %2, %%xmm0, %%xmm1\n\t"
        "vmovups %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(xmm_init[0]), "m"(mem[0]) : "xmm0","xmm1"
    );
    TEST_ASSERT(r[0] == 1.0f, "vmovhps load r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 2.0f, "vmovhps load r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == 3.0f, "vmovhps load r[2]=%f", r[2]);
    TEST_ASSERT(r[3] == 4.0f, "vmovhps load r[3]=%f", r[3]);
    /* store */
    float out[2];
    __asm__ volatile(
        "vmovups %1, %%xmm0\n\t"
        "vmovhps %%xmm0, %0\n\t"
        : "=m"(out[0]) : "m"(r[0]) : "xmm0"
    );
    TEST_ASSERT(out[0] == 3.0f, "vmovhps store out[0]=%f", out[0]);
    TEST_ASSERT(out[1] == 4.0f, "vmovhps store out[1]=%f", out[1]);
}

static void test_vmovlpd(void) {
    TEST_START("VMOVLPD (load/store low 64-bit double)");
    double lo = 5.0, hi = 6.0;
    double r[2];
    __asm__ volatile(
        "vmovsd %2, %%xmm0\n\t"       /* xmm0[63:0]=hi (placeholder) */
        "vmovhpd %2, %%xmm0, %%xmm0\n\t" /* set high=hi */
        "vmovlpd %1, %%xmm0, %%xmm1\n\t" /* xmm1[63:0]=lo, high unchanged */
        "vmovupd %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(lo), "m"(hi) : "xmm0","xmm1"
    );
    TEST_ASSERT(r[0] == 5.0, "vmovlpd load r[0]=%f", r[0]);
    /* store */
    double out;
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovlpd %%xmm0, %0\n\t"
        : "=m"(out) : "m"(r[0]) : "xmm0"
    );
    TEST_ASSERT(out == 5.0, "vmovlpd store out=%f", out);
}

static void test_vmovlps(void) {
    TEST_START("VMOVLPS (load/store low 64-bit as two floats)");
    float mem[2] = {7.0f, 8.0f};
    float xmm_init[4] = {0.0f, 0.0f, 9.0f, 10.0f};
    float r[4];
    __asm__ volatile(
        "vmovups %1, %%xmm0\n\t"
        "vmovlps %2, %%xmm0, %%xmm1\n\t"
        "vmovups %%xmm1, %0\n\t"
        : "=m"(r[0]) : "m"(xmm_init[0]), "m"(mem[0]) : "xmm0","xmm1"
    );
    TEST_ASSERT(r[0] == 7.0f,  "vmovlps load r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 8.0f,  "vmovlps load r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == 9.0f,  "vmovlps load r[2]=%f", r[2]);
    TEST_ASSERT(r[3] == 10.0f, "vmovlps load r[3]=%f", r[3]);
    /* store */
    float out[2];
    __asm__ volatile(
        "vmovups %1, %%xmm0\n\t"
        "vmovlps %%xmm0, %0\n\t"
        : "=m"(out[0]) : "m"(r[0]) : "xmm0"
    );
    TEST_ASSERT(out[0] == 7.0f, "vmovlps store out[0]=%f", out[0]);
    TEST_ASSERT(out[1] == 8.0f, "vmovlps store out[1]=%f", out[1]);
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vmovsd();
    test_vmovss();
    test_vmovhpd();
    test_vmovhps();
    test_vmovlpd();
    test_vmovlps();
    TEST_END();
}
