/*
 * Test VANDPD/VANDPS/VANDNPD/VANDNPS/VORPD/VORPS/VXORPD/VXORPS
 * 256-bit bitwise logical operations on packed float/double
 * Compile: gcc -o test_vlogical avx256/test_vlogical.c -O0 -mavx2
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

static void test_vandpd(void) {
    TEST_START("VANDPD (256-bit)");
    uint64_t a[4] = {0xFFFFFFFFFFFFFFFFULL, 0x0F0F0F0F0F0F0F0FULL,
                     0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL};
    uint64_t b[4] = {0x0F0F0F0F0F0F0F0FULL, 0xFFFFFFFFFFFFFFFFULL,
                     0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL};
    uint64_t r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vandpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0x0F0F0F0F0F0F0F0FULL, "vandpd r[0]=%016llx", (unsigned long long)r[0]);
    TEST_ASSERT(r[1] == 0x0F0F0F0F0F0F0F0FULL, "vandpd r[1]=%016llx", (unsigned long long)r[1]);
    TEST_ASSERT(r[2] == 0x0000000000000000ULL, "vandpd r[2]=%016llx", (unsigned long long)r[2]);
    TEST_ASSERT(r[3] == 0x0000000000000000ULL, "vandpd r[3]=%016llx", (unsigned long long)r[3]);
    /* mem form */
    uint64_t r2[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vandpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 0x0F0F0F0F0F0F0F0FULL, "vandpd mem r[0]=%016llx", (unsigned long long)r2[0]);
}

static void test_vandps(void) {
    TEST_START("VANDPS (256-bit)");
    uint32_t a[8] = {0xFFFFFFFF,0x0F0F0F0F,0xAAAAAAAA,0x55555555,
                     0xFFFFFFFF,0x0F0F0F0F,0xAAAAAAAA,0x55555555};
    uint32_t b[8] = {0x0F0F0F0F,0xFFFFFFFF,0x55555555,0xAAAAAAAA,
                     0x0F0F0F0F,0xFFFFFFFF,0x55555555,0xAAAAAAAA};
    uint32_t r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vandps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0x0F0F0F0FU, "vandps r[0]=%08x", r[0]);
    TEST_ASSERT(r[2] == 0x00000000U, "vandps r[2]=%08x", r[2]);
}

static void test_vandnpd(void) {
    TEST_START("VANDNPD (256-bit)");
    /* vandnpd dst, src1, src2 => dst = (~src1) & src2 */
    uint64_t a[4] = {0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL,
                     0x0000000000000000ULL,0x0000000000000000ULL};
    uint64_t b[4] = {0xAAAAAAAAAAAAAAAAULL,0x5555555555555555ULL,
                     0xAAAAAAAAAAAAAAAAULL,0x5555555555555555ULL};
    uint64_t r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vandnpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0x0000000000000000ULL, "vandnpd r[0]=%016llx", (unsigned long long)r[0]);
    TEST_ASSERT(r[2] == 0xAAAAAAAAAAAAAAAAULL, "vandnpd r[2]=%016llx", (unsigned long long)r[2]);
}

static void test_vandnps(void) {
    TEST_START("VANDNPS (256-bit)");
    uint32_t a[8] = {0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00000000,
                     0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x00000000};
    uint32_t b[8] = {0xAAAAAAAA,0x55555555,0xAAAAAAAA,0x55555555,
                     0xAAAAAAAA,0x55555555,0xAAAAAAAA,0x55555555};
    uint32_t r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vandnps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0x00000000U, "vandnps r[0]=%08x", r[0]);
    TEST_ASSERT(r[2] == 0xAAAAAAAAU, "vandnps r[2]=%08x", r[2]);
}

static void test_vorpd(void) {
    TEST_START("VORPD (256-bit)");
    uint64_t a[4] = {0xAAAAAAAAAAAAAAAAULL,0x5555555555555555ULL,
                     0x0000000000000000ULL,0xFFFFFFFFFFFFFFFFULL};
    uint64_t b[4] = {0x5555555555555555ULL,0xAAAAAAAAAAAAAAAAULL,
                     0xFFFFFFFFFFFFFFFFULL,0x0000000000000000ULL};
    uint64_t r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vorpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0xFFFFFFFFFFFFFFFFULL, "vorpd r[0]=%016llx", (unsigned long long)r[0]);
    TEST_ASSERT(r[2] == 0xFFFFFFFFFFFFFFFFULL, "vorpd r[2]=%016llx", (unsigned long long)r[2]);
    /* mem form */
    uint64_t r2[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vorpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 0xFFFFFFFFFFFFFFFFULL, "vorpd mem r[0]=%016llx", (unsigned long long)r2[0]);
}

static void test_vorps(void) {
    TEST_START("VORPS (256-bit)");
    uint32_t a[8] = {0xAAAAAAAA,0x55555555,0x00000000,0xFFFFFFFF,
                     0xAAAAAAAA,0x55555555,0x00000000,0xFFFFFFFF};
    uint32_t b[8] = {0x55555555,0xAAAAAAAA,0xFFFFFFFF,0x00000000,
                     0x55555555,0xAAAAAAAA,0xFFFFFFFF,0x00000000};
    uint32_t r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vorps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0xFFFFFFFFU, "vorps r[0]=%08x", r[0]);
    TEST_ASSERT(r[3] == 0xFFFFFFFFU, "vorps r[3]=%08x", r[3]);
}

static void test_vxorpd(void) {
    TEST_START("VXORPD (256-bit)");
    uint64_t a[4] = {0xAAAAAAAAAAAAAAAAULL,0x5555555555555555ULL,
                     0xFFFFFFFFFFFFFFFFULL,0x0000000000000000ULL};
    uint64_t b[4] = {0x5555555555555555ULL,0x5555555555555555ULL,
                     0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL};
    uint64_t r[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovupd %2, %%ymm1\n\t"
        "vxorpd %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0xFFFFFFFFFFFFFFFFULL, "vxorpd r[0]=%016llx", (unsigned long long)r[0]);
    TEST_ASSERT(r[1] == 0x0000000000000000ULL, "vxorpd r[1]=%016llx", (unsigned long long)r[1]);
    TEST_ASSERT(r[2] == 0x0000000000000000ULL, "vxorpd r[2]=%016llx", (unsigned long long)r[2]);
    TEST_ASSERT(r[3] == 0xFFFFFFFFFFFFFFFFULL, "vxorpd r[3]=%016llx", (unsigned long long)r[3]);
    /* mem form */
    uint64_t r2[4];
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vxorpd %2, %%ymm0, %%ymm2\n\t"
        "vmovupd %%ymm2, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r2[0] == 0xFFFFFFFFFFFFFFFFULL, "vxorpd mem r[0]=%016llx", (unsigned long long)r2[0]);
}

static void test_vxorps(void) {
    TEST_START("VXORPS (256-bit)");
    uint32_t a[8] = {0xAAAAAAAA,0x55555555,0xFFFFFFFF,0x00000000,
                     0xAAAAAAAA,0x55555555,0xFFFFFFFF,0x00000000};
    uint32_t b[8] = {0x55555555,0x55555555,0xFFFFFFFF,0xFFFFFFFF,
                     0x55555555,0x55555555,0xFFFFFFFF,0xFFFFFFFF};
    uint32_t r[8];
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vxorps %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 0xFFFFFFFFU, "vxorps r[0]=%08x", r[0]);
    TEST_ASSERT(r[1] == 0x00000000U, "vxorps r[1]=%08x", r[1]);
    TEST_ASSERT(r[2] == 0x00000000U, "vxorps r[2]=%08x", r[2]);
    TEST_ASSERT(r[3] == 0xFFFFFFFFU, "vxorps r[3]=%08x", r[3]);
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vandpd();
    test_vandps();
    test_vandnpd();
    test_vandnps();
    test_vorpd();
    test_vorps();
    test_vxorpd();
    test_vxorps();
    TEST_END();
}
