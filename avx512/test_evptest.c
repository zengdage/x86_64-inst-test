/*
 * Test VPTESTMD/VPTESTMQ with zmm registers (AVX-512F).
 * Packed integer test producing mask register results.
 *
 * Compile: gcc -o test_evptest avx512/test_evptest.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 16) & 1;
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPTEST (VPTESTMD/VPTESTMQ zmm -> k)");

    zmm_t a, b;
    uint64_t kmask;

    /* VPTESTMD: set k bit if (a[i] AND b[i]) != 0 */
    /* All lanes non-zero AND */
    for (int i = 0; i < 16; i++) { a.u32[i] = 0xFFFFFFFFU; b.u32[i] = 0xFFFFFFFFU; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vptestmd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0xFFFF, "VPTESTMD all-ones: mask=%04llx", (unsigned long long)kmask);

    /* All lanes zero AND */
    for (int i = 0; i < 16; i++) { a.u32[i] = 0xAAAAAAAAU; b.u32[i] = 0x55555555U; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vptestmd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0, "VPTESTMD all-zero AND: mask=%04llx", (unsigned long long)kmask);

    /* Alternating: even lanes have bits set */
    for (int i = 0; i < 16; i++) {
        a.u32[i] = (i % 2 == 0) ? 0xFFFFFFFFU : 0;
        b.u32[i] = 0xFFFFFFFFU;
    }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vptestmd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0x5555, "VPTESTMD alternating: mask=%04llx", (unsigned long long)kmask);

    /* VPTESTMQ: 8 qword lanes */
    for (int i = 0; i < 8; i++) { a.u64[i] = (uint64_t)(i + 1); b.u64[i] = 0xFFFFFFFFFFFFFFFFULL; }
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vptestmq %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0xFF, "VPTESTMQ all-nonzero: mask=%02llx", (unsigned long long)kmask);

    /* VPTESTMQ: first lane zero */
    a.u64[0] = 0;
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vptestmq %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0","zmm1","k1"
    );
    TEST_ASSERT(kmask == 0xFE, "VPTESTMQ first-zero: mask=%02llx", (unsigned long long)kmask);

    for (int i = 0; i < 16; i++) { a.u32[i] = 0; b.u32[i] = UINT32_MAX; }
    a.u32[0] = 1; a.u32[15] = 0x80000000u;
    uint64_t gate = 0xffff;
    __asm__ volatile ("kmovq %3,%%k2\n\tvmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvptestmd %%zmm1,%%zmm0,%%k1%{%%k2%}\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b), "r"(gate) : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT((kmask & 0xffff) == 0x8001, "VPTESTMD lowest/highest mask bits: %04llx", (unsigned long long)kmask);
    gate = 0;
    __asm__ volatile ("kmovq %3,%%k2\n\tvmovdqu32 %1,%%zmm0\n\tvmovdqu32 %2,%%zmm1\n\tvptestmd %%zmm1,%%zmm0,%%k1%{%%k2%}\n\tkmovq %%k1,%0"
        : "=r"(kmask) : "m"(a), "m"(b), "r"(gate) : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT((kmask & 0xffff) == 0, "VPTESTMD k=0 input writemask");

    /* VPTESTNMD/VPTESTNMQ set a result bit when the lane-wise AND is zero. */
    for (int i = 0; i < 16; i++) {
        a.u32[i] = UINT32_C(0xffffffff);
        b.u32[i] = UINT32_C(0x80000001);
    }
    a.u32[0] = UINT32_C(0x7ffffffe);
    a.u32[15] = 0;
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vptestnmd %%zmm1, %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1"
    );
    TEST_ASSERT((kmask & 0xffff) == 0x8001,
                "VPTESTNMD zero-AND lowest/highest lanes: %04llx",
                (unsigned long long)(kmask & 0xffff));

    for (int i = 0; i < 8; i++) {
        a.u64[i] = UINT64_C(0xffffffffffffffff);
        b.u64[i] = UINT64_C(0x0100000000000080);
    }
    a.u64[0] = UINT64_C(0xfeffffffffffff7f);
    a.u64[7] = 0;
    uint64_t qgate = 0x81;
    __asm__ volatile (
        "kmovq %3, %%k2\n\t"
        "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vptestnmq %%zmm1, %%zmm0, %%k1%{%%k2%}\n\t"
        "kmovq %%k1, %0"
        : "=r"(kmask) : "m"(a), "m"(b), "r"(qgate)
        : "zmm0", "zmm1", "k1", "k2"
    );
    TEST_ASSERT((kmask & 0xff) == 0x81,
                "VPTESTNMQ endpoint results through endpoint writemask: %02llx",
                (unsigned long long)(kmask & 0xff));

    TEST_END();
}
