/*
 * Test VPSLLW/VPSLLD/VPSLLQ with zmm registers (AVX-512BW/F).
 * Packed integer logical left shift (word/dword/qword).
 *
 * Compile: gcc -o test_evpsll avx512/test_evpsll.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

#define RUN_ZSHIFT_IMM(op, count, src, out) do { \
    __asm__ volatile("vmovdqu64 %1,%%zmm0\n\t" #op " $" #count ",%%zmm0,%%zmm1\n\tvmovdqu64 %%zmm1,%0" \
        : "=m"(out) : "m"(src) : "zmm0", "zmm1"); \
} while (0)

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512F/BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPSLLW/EVPSLLD/EVPSLLQ (VPSLLW/VPSLLD/VPSLLQ zmm)");

    zmm_t a, dst;

    /* VPSLLW: shift left by 3 */
    for (int i = 0; i < 32; i++) a.u16[i] = (uint16_t)(i + 1);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vpsllw $3, %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == (uint16_t)(a.u16[i] << 3),
            "VPSLLW lane %d: %u != %u", i, dst.u16[i], (uint16_t)(a.u16[i] << 3));

    /* VPSLLD: shift left by 4 */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i + 1);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpslld $4, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == a.u32[i] << 4,
            "VPSLLD lane %d: %u != %u", i, dst.u32[i], a.u32[i] << 4);

    /* VPSLLQ: shift left by 8 */
    for (int i = 0; i < 8; i++) a.u64[i] = (uint64_t)(i + 1);
    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vpsllq $8, %%zmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == a.u64[i] << 8,
            "VPSLLQ lane %d: %llu != %llu", i,
            (unsigned long long)dst.u64[i], (unsigned long long)(a.u64[i] << 8));

    /* VPSLLD variable shift using xmm count */
    for (int i = 0; i < 16; i++) a.u32[i] = 1;
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpxor %%xmm1, %%xmm1, %%xmm1\n\t"
        "vpinsrd $0, %2, %%xmm1, %%xmm1\n\t"
        "vpslld %%xmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "r"(5) : "zmm0","xmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 32, "VPSLLD var shift lane %d: %u", i, dst.u32[i]);

    /* Element-width and maximum-imm8 boundaries zero every logical lane. */
    memset(&a, 0xff, sizeof(a));
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vpsllw $16, %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0", "zmm1"
    );
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(dst.u16[i] == 0, "VPSLLW shift-width lane %d", i);

    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpslld $32, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0", "zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0, "VPSLLD shift-width lane %d", i);

    __asm__ volatile (
        "vmovdqu64 %1, %%zmm0\n\t"
        "vpsllq $0xff, %%zmm0, %%zmm1\n\t"
        "vmovdqu64 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0", "zmm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == 0, "VPSLLQ max-imm8 lane %d", i);

    /* Boundary: shift by 0 */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i * 7 + 1);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpslld $0, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == a.u32[i], "VPSLLD shift-0 lane %d: %u", i, dst.u32[i]);

    /* VPSLLD with zeroing masking */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)(i + 1);
    uint64_t kmask = 0x0F0F;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpslld $2, %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a), "m"(a), "r"(kmask) : "zmm0","zmm1","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? a.u32[i] << 2 : 0;
        TEST_ASSERT(dst.u32[i] == expected,
            "VPSLLD zero mask lane %d: %u != %u", i, dst.u32[i], expected);
    }

    memset(&a, 0xff, sizeof(a));
    RUN_ZSHIFT_IMM(vpsllw, 15, a, dst);
    for (int i = 0; i < 32; i++) TEST_ASSERT(dst.u16[i] == 0x8000u, "VPSLLW width-1 lane %d", i);
    RUN_ZSHIFT_IMM(vpsllw, 17, a, dst);
    for (int i = 0; i < 32; i++) TEST_ASSERT(dst.u16[i] == 0, "VPSLLW width+1 lane %d", i);
    RUN_ZSHIFT_IMM(vpslld, 31, a, dst);
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == 0x80000000u, "VPSLLD width-1 lane %d", i);
    RUN_ZSHIFT_IMM(vpslld, 33, a, dst);
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == 0, "VPSLLD width+1 lane %d", i);
    RUN_ZSHIFT_IMM(vpsllq, 63, a, dst);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == UINT64_C(0x8000000000000000), "VPSLLQ width-1 lane %d", i);
    RUN_ZSHIFT_IMM(vpsllq, 65, a, dst);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst.u64[i] == 0, "VPSLLQ width+1 lane %d", i);

    uint64_t count_high_ignored[2] = {1, UINT64_MAX};
    uint64_t count_low_max[2] = {UINT64_MAX, 0};
    __asm__ volatile ("vmovdqu64 %1,%%zmm0\n\tvmovdqu %2,%%xmm1\n\tvpslld %%xmm1,%%zmm0,%%zmm2\n\tvmovdqu64 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(count_high_ignored) : "zmm0", "xmm1", "zmm2");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == 0xfffffffeu, "VPSLLD ignores count high qword lane %d", i);
    __asm__ volatile ("vmovdqu64 %1,%%zmm0\n\tvmovdqu %2,%%xmm1\n\tvpslld %%xmm1,%%zmm0,%%zmm2\n\tvmovdqu64 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(count_low_max) : "zmm0", "xmm1", "zmm2");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == 0, "VPSLLD UINT64_MAX count lane %d", i);

    kmask = 0x8001;
    zmm_t initial; for (int i = 0; i < 16; i++) initial.u32[i] = 0xdeadbeefu;
    __asm__ volatile ("kmovq %4,%%k1\n\tvmovdqu32 %1,%%zmm0\n\tvmovdqu32 %3,%%zmm1\n\tvpslld $1,%%zmm0,%%zmm1%{%%k1%}\n\tvmovdqu32 %%zmm1,%0"
        : "=m"(dst) : "m"(a), "m"(a), "m"(initial), "r"(kmask) : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 16; i++) TEST_ASSERT(dst.u32[i] == ((i == 0 || i == 15) ? 0xfffffffeu : 0xdeadbeefu), "VPSLLD merge mask lane %d", i);

    TEST_END();
}
