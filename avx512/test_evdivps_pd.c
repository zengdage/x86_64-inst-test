/*
 * Test VDIVPS/VDIVPD with zmm registers (AVX-512F).
 * Packed single/double precision floating-point division.
 *
 * Compile: gcc -o test_evdivps_pd avx512/test_evdivps_pd.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
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
    TEST_START("EVDIVPS/EVDIVPD (VDIVPS/VDIVPD zmm)");

    zmm_t a, b, dst;

    /* VDIVPS basic */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)((i + 1) * 4); b.f32[i] = (float)(i + 1); }
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vdivps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == 4.0f, "VDIVPS lane %d: %f != 4.0", i, dst.f32[i]);

    /* VDIVPD basic */
    for (int i = 0; i < 8; i++) { a.f64[i] = (double)((i + 1) * 8); b.f64[i] = (double)(i + 1); }
    __asm__ volatile (
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vdivpd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovapd %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.f64[i] == 8.0, "VDIVPD lane %d: %f != 8.0", i, dst.f64[i]);

    /* VDIVPS with zeroing masking {k1}{z} */
    for (int i = 0; i < 16; i++) { a.f32[i] = (float)((i + 1) * 2); b.f32[i] = 2.0f; }
    uint64_t kmask = 0xF0F0;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vdivps %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        float expected = (kmask >> i) & 1 ? (float)(i + 1) : 0.0f;
        TEST_ASSERT(dst.f32[i] == expected, "VDIVPS zero mask lane %d: %f != %f", i, dst.f32[i], expected);
    }

    /* Boundary: divide by self = 1 */
    for (int i = 0; i < 16; i++) a.f32[i] = (float)(i + 1) * 3.14f;
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vdivps %%zmm0, %%zmm0, %%zmm1\n\t"
        "vmovaps %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.f32[i] == 1.0f, "VDIVPS self-div lane %d: %f", i, dst.f32[i]);

    /* IEEE-754 divide boundaries. */
    for (int i = 0; i < 16; i++) { a.f32[i] = 1.0f; b.f32[i] = 1.0f; }
    a.f32[0] = 1.0f;      b.f32[0] = 0.0f;
    a.f32[1] = -1.0f;     b.f32[1] = 0.0f;
    a.f32[2] = 0.0f;      b.f32[2] = 0.0f;
    a.f32[3] = INFINITY;  b.f32[3] = INFINITY;
    a.f32[4] = 1.0f;      b.f32[4] = INFINITY;
    __asm__ volatile (
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vdivps %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2"
    );
    TEST_ASSERT(isinf(dst.f32[0]) && dst.f32[0] > 0.0f, "VDIVPS 1/+0 = +inf");
    TEST_ASSERT(isinf(dst.f32[1]) && dst.f32[1] < 0.0f, "VDIVPS -1/+0 = -inf");
    TEST_ASSERT(IS_QNAN(dst.f32[2]) && IS_QNAN(dst.f32[3]),
                "VDIVPS 0/0 and inf/inf produce QNaNs");
    TEST_ASSERT(dst.f32[4] == 0.0f && !signbit(dst.f32[4]), "VDIVPS 1/+inf = +0");

#if ENABLE_MXCSR_CHECK
    /* Masked-off zero divisors must not set the MXCSR divide-by-zero flag. */
    for (int i = 0; i < 16; i++) { a.f32[i] = 1.0f; b.f32[i] = 0.0f; }
    kmask = 0;
    uint32_t mxcsr_before, mxcsr_after, mxcsr_clean;
    __asm__ volatile ("stmxcsr %0" : "=m"(mxcsr_before));
    mxcsr_clean = mxcsr_before & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr_clean));
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vdivps %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovaps %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1"
    );
    __asm__ volatile ("stmxcsr %0" : "=m"(mxcsr_after));
    __asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr_before));
    TEST_ASSERT(!(mxcsr_after & (1u << 2)), "VDIVPS empty mask suppresses divide-by-zero exception");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0, "VDIVPS empty zero mask lane %d", i);
#endif

    TEST_END();
}
