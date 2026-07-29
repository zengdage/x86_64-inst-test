/*
 * Test VDPPD/VDPPPS
 * VDPPD: 128-bit xmm dot product of packed doubles (imm8 controls input/output mask)
 * VDPPPS: 256-bit ymm dot product of packed floats
 * Compile: gcc -o test_vdppd_dpps avx256/test_vdppd_dpps.c -O0 -mavx2
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

static void test_vdppd(void) {
    TEST_START("VDPPD (128-bit xmm dot product of doubles)");
    /* imm8: bits[5:4] = input mask (which elements to multiply), bits[1:0] = output mask */
    /* imm8=0x31: input mask=0b11 (both), output mask=0b01 (lane0 only) */
    double a[2] = {2.0, 3.0};
    double b[2] = {4.0, 5.0};
    double r[2];
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovupd %2, %%xmm1\n\t"
        "vdppd $0x31, %%xmm1, %%xmm0, %%xmm2\n\t"  /* dot=2*4+3*5=23, out to lane0 */
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r[0] == 23.0, "vdppd 0x31 r[0]=%f (2*4+3*5=23)", r[0]);
    TEST_ASSERT(r[1] == 0.0,  "vdppd 0x31 r[1]=%f (output mask bit1=0)", r[1]);

    /* imm8=0x33: output to both lanes */
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovupd %2, %%xmm1\n\t"
        "vdppd $0x33, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r[0] == 23.0, "vdppd 0x33 r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 23.0, "vdppd 0x33 r[1]=%f", r[1]);

    /* imm8=0x11: only lane0 input, lane0 output: 2*4=8 */
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovupd %2, %%xmm1\n\t"
        "vdppd $0x11, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r[0] == 8.0, "vdppd 0x11 r[0]=%f (2*4=8)", r[0]);

    /* mem operand form */
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vdppd $0x31, %2, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm2"
    );
    TEST_ASSERT(r[0] == 23.0, "vdppd mem r[0]=%f", r[0]);
}

static void test_vdppps(void) {
    TEST_START("VDPPPS (256-bit ymm dot product of floats)");
    /* 256-bit: operates on two independent 128-bit halves */
    /* imm8=0xFF: all inputs, all outputs */
    float a[8] = {1,2,3,4, 1,2,3,4};
    float b[8] = {5,6,7,8, 5,6,7,8};
    float r[8];
    /* dot per 128-bit half: 1*5+2*6+3*7+4*8 = 5+12+21+32 = 70 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vdpps $0xFF, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 70.0f, "vdpps 0xFF r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 70.0f, "vdpps 0xFF r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == 70.0f, "vdpps 0xFF r[2]=%f", r[2]);
    TEST_ASSERT(r[3] == 70.0f, "vdpps 0xFF r[3]=%f", r[3]);
    TEST_ASSERT(r[4] == 70.0f, "vdpps 0xFF r[4]=%f (upper half)", r[4]);

    /* imm8=0xF1: all inputs, only lane0 of each 128-bit half */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vdpps $0xF1, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 70.0f, "vdpps 0xF1 r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 0.0f,  "vdpps 0xF1 r[1]=%f (output mask)", r[1]);
    TEST_ASSERT(r[4] == 70.0f, "vdpps 0xF1 r[4]=%f (upper half lane0)", r[4]);
    TEST_ASSERT(r[5] == 0.0f,  "vdpps 0xF1 r[5]=%f (upper half masked)", r[5]);

    /* mem operand form */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vdpps $0xFF, %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r[0] == 70.0f, "vdpps mem r[0]=%f", r[0]);
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vdppd();
    test_vdppps();
    TEST_END();
}
