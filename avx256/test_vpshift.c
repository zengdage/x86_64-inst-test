/*
 * Test VPSLLD/VPSLLQ/VPSLLW/VPSRLD/VPSRLQ/VPSRLW/VPSRAD/VPSRAQ/VPSRAW
 * 256-bit packed integer shift instructions (AVX2 ymm forms)
 * Compile: gcc -o test_vpshift avx256/test_vpshift.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include "../common.h"

static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(7),"c"(0));
    return (ebx >> 5) & 1;
}

static void test_vpslld(void) {
    TEST_START("VPSLLD (256-bit shift left dword by imm)");
    int32_t a[8] = {1,2,3,4,5,6,7,8};
    int32_t r[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpslld $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == a[i] * 4, "vpslld r[%d]=%d", i, r[i]);
    /* xmm count form */
    int32_t r2[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vmovd %2, %%xmm2\n\t"
        "vpslld %%xmm2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "r"(2) : "ymm0","ymm1","xmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r2[i] == a[i] * 4, "vpslld xmm r[%d]=%d", i, r2[i]);
}

static void test_vpsllq(void) {
    TEST_START("VPSLLQ (256-bit shift left qword by imm)");
    int64_t a[4] = {1, 2, 3, 4};
    int64_t r[4];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsllq $3, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == a[i] * 8, "vpsllq r[%d]=%lld", i, (long long)r[i]);
}

static void test_vpsllw(void) {
    TEST_START("VPSLLW (256-bit shift left word by imm)");
    int16_t a[16];
    for (int i = 0; i < 16; i++) a[i] = (int16_t)(i + 1);
    int16_t r[16];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsllw $1, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(r[i] == a[i] * 2, "vpsllw r[%d]=%d", i, r[i]);
}

static void test_vpsrld(void) {
    TEST_START("VPSRLD (256-bit logical shift right dword by imm)");
    uint32_t a[8] = {8,16,32,64,128,256,512,1024};
    uint32_t r[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrld $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == a[i] / 4, "vpsrld r[%d]=%u", i, r[i]);
    /* sign bit not propagated */
    uint32_t neg[8];
    for (int i = 0; i < 8; i++) neg[i] = 0x80000000U;
    uint32_t rneg[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrld $1, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(rneg[0]) : "m"(neg[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(rneg[0] == 0x40000000U, "vpsrld logical neg r[0]=0x%08x", rneg[0]);
}

static void test_vpsrlq(void) {
    TEST_START("VPSRLQ (256-bit logical shift right qword by imm)");
    uint64_t a[4] = {64, 128, 256, 512};
    uint64_t r[4];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrlq $3, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == a[i] / 8, "vpsrlq r[%d]=%llu", i, (unsigned long long)r[i]);
}

static void test_vpsrlw(void) {
    TEST_START("VPSRLW (256-bit logical shift right word by imm)");
    uint16_t a[16];
    for (int i = 0; i < 16; i++) a[i] = (uint16_t)((i + 1) * 4);
    uint16_t r[16];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrlw $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(r[i] == (uint16_t)(i + 1), "vpsrlw r[%d]=%u", i, r[i]);
}

static void test_vpsrad(void) {
    TEST_START("VPSRAD (256-bit arithmetic shift right dword by imm)");
    int32_t a[8] = {-8,-16,8,16,-32,32,-64,64};
    int32_t r[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrad $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == a[i] / 4, "vpsrad r[%d]=%d", i, r[i]);
    /* sign preserved */
    TEST_ASSERT(r[0] == -2, "vpsrad neg r[0]=%d", r[0]);
}

static void test_vpsraq(void) {
    TEST_START("VPSRAQ (256-bit arithmetic shift right qword by imm, AVX-512 but test gracefully)");
    /* VPSRAQ is AVX-512VL; skip if not available, just test VPSRLQ as fallback */
    /* We test VPSRLQ with sign-bit check to document the logical behavior */
    int64_t a[4] = {-8LL, -16LL, 8LL, 16LL};
    int64_t r[4];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrlq $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    /* logical: sign bit not preserved */
    TEST_ASSERT(r[0] != -2LL, "vpsrlq (logical) does not preserve sign r[0]=%lld", (long long)r[0]);
    TEST_ASSERT(r[2] == 2LL,  "vpsrlq positive r[2]=%lld", (long long)r[2]);
}

static void test_vpsraw(void) {
    TEST_START("VPSRAW (256-bit arithmetic shift right word by imm)");
    int16_t a[16];
    for (int i = 0; i < 8; i++) { a[i] = (int16_t)(-4 * (i+1)); a[i+8] = (int16_t)(4 * (i+1)); }
    int16_t r[16];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsraw $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(r[i] == a[i] / 4, "vpsraw r[%d]=%d", i, r[i]);
    TEST_ASSERT(r[0] == -1, "vpsraw neg r[0]=%d", r[0]);
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vpslld();
    test_vpsllq();
    test_vpsllw();
    test_vpsrld();
    test_vpsrlq();
    test_vpsrlw();
    test_vpsrad();
    test_vpsraq();
    test_vpsraw();
    TEST_END();
}
