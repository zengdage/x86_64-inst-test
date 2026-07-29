/*
 * Test VPCLMULQDQ with zmm registers (AVX-512 + VPCLMULQDQ extension).
 * Carry-less multiplication of 64-bit polynomials.
 *
 * Compile: gcc -o test_evpclmulqdq avx512/test_evpclmulqdq.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl -mvpclmulqdq
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

static int check_pclmul(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
    return (ecx >> 1) & 1; /* PCLMULQDQ: CPUID.1.ECX[1] */
}
static int check_vpclmulqdq(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ecx >> 10) & 1; /* VPCLMULQDQ: CPUID.7.ECX[10] */
}

/* Software carry-less multiply for verification */
static void clmul_ref(uint64_t a, uint64_t b, uint64_t *lo, uint64_t *hi) {
    *lo = 0; *hi = 0;
    for (int i = 0; i < 64; i++) {
        if ((b >> i) & 1) {
            *lo ^= a << i;
            if (i > 0) *hi ^= a >> (64 - i);
        }
    }
}

int main(void) {
    if (!check_pclmul()) {
        printf("PCLMULQDQ not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPCLMULQDQ (VPCLMULQDQ xmm/zmm)");

    xmm_t a, b, dst;

    /* VPCLMULQDQ xmm: imm8=0x00 -> lo(a)*lo(b) */
    a.u64[0] = 0x0000000000000001ULL; a.u64[1] = 0;
    b.u64[0] = 0x0000000000000001ULL; b.u64[1] = 0;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(dst.u64[0] == 1 && dst.u64[1] == 0, "VPCLMULQDQ 1*1=1: %llx", (unsigned long long)dst.u64[0]);

    /* VPCLMULQDQ xmm: 2*2 = 4 in GF(2) */
    a.u64[0] = 2; b.u64[0] = 2;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(dst.u64[0] == 4 && dst.u64[1] == 0, "VPCLMULQDQ 2*2=4: %llx", (unsigned long long)dst.u64[0]);

    /* VPCLMULQDQ xmm: verify against software reference */
    a.u64[0] = 0xDEADBEEFCAFEBABEULL; a.u64[1] = 0;
    b.u64[0] = 0x0123456789ABCDEFULL; b.u64[1] = 0;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x00, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    uint64_t ref_lo, ref_hi;
    clmul_ref(a.u64[0], b.u64[0], &ref_lo, &ref_hi);
    TEST_ASSERT(dst.u64[0] == ref_lo, "VPCLMULQDQ lo: %016llx != %016llx",
        (unsigned long long)dst.u64[0], (unsigned long long)ref_lo);
    TEST_ASSERT(dst.u64[1] == ref_hi, "VPCLMULQDQ hi: %016llx != %016llx",
        (unsigned long long)dst.u64[1], (unsigned long long)ref_hi);

    /* VPCLMULQDQ imm8=0x11: hi(a)*hi(b) */
    a.u64[0] = 0; a.u64[1] = 0xDEADBEEFCAFEBABEULL;
    b.u64[0] = 0; b.u64[1] = 0x0123456789ABCDEFULL;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vpclmulqdq $0x11, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(dst.u64[0] == ref_lo, "VPCLMULQDQ hi*hi lo: %016llx", (unsigned long long)dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == ref_hi, "VPCLMULQDQ hi*hi hi: %016llx", (unsigned long long)dst.u64[1]);

    /* VPCLMULQDQ zmm (VPCLMULQDQ extension) */
    if (!check_vpclmulqdq()) {
        printf("VPCLMULQDQ (zmm) not supported, skipping zmm tests.\n");
    } else {
        zmm_t za, zb, zdst;
        for (int i = 0; i < 4; i++) {
            za.u64[i*2]   = 0xDEADBEEFCAFEBABEULL;
            za.u64[i*2+1] = 0;
            zb.u64[i*2]   = 0x0123456789ABCDEFULL;
            zb.u64[i*2+1] = 0;
        }
        __asm__ volatile (
            "vmovdqu64 %1, %%zmm0\n\t"
            "vmovdqu64 %2, %%zmm1\n\t"
            "vpclmulqdq $0x00, %%zmm1, %%zmm0, %%zmm2\n\t"
            "vmovdqu64 %%zmm2, %0"
            : "=m"(zdst) : "m"(za), "m"(zb) : "zmm0","zmm1","zmm2"
        );
        for (int i = 0; i < 4; i++) {
            TEST_ASSERT(zdst.u64[i*2]   == ref_lo, "VPCLMULQDQ zmm lane%d lo: %016llx", i, (unsigned long long)zdst.u64[i*2]);
            TEST_ASSERT(zdst.u64[i*2+1] == ref_hi, "VPCLMULQDQ zmm lane%d hi: %016llx", i, (unsigned long long)zdst.u64[i*2+1]);
        }
    }

    TEST_END();
}
