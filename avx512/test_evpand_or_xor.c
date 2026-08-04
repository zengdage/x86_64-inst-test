/*
 * Test VPANDD/VPANDND/VPORD/VPXORD with zmm registers (AVX-512F).
 * Packed bitwise AND/ANDN/OR/XOR on 32-bit dword elements.
 *
 * Compile: gcc -o test_evpand_or_xor avx512/test_evpand_or_xor.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
    TEST_START("EVPAND/EVPANDN/EVPOR/EVPXOR (VPANDD/VPANDND/VPORD/VPXORD zmm)");

    zmm_t a, b, dst;

    /* VPANDD */
    for (int i = 0; i < 16; i++) { a.u32[i] = 0xF0F0F0F0U; b.u32[i] = 0xFF00FF00U; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpandd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == (a.u32[i] & b.u32[i]),
            "VPANDD lane %d: %08x != %08x", i, dst.u32[i], a.u32[i] & b.u32[i]);

    /* VPANDND: dst = ~a & b */
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpandnd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == (~a.u32[i] & b.u32[i]),
            "VPANDND lane %d: %08x != %08x", i, dst.u32[i], ~a.u32[i] & b.u32[i]);

    /* VPORD */
    for (int i = 0; i < 16; i++) { a.u32[i] = 0x0F0F0F0FU; b.u32[i] = 0xF0F0F0F0U; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpord %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == (a.u32[i] | b.u32[i]),
            "VPORD lane %d: %08x != %08x", i, dst.u32[i], a.u32[i] | b.u32[i]);

    /* VPXORD */
    for (int i = 0; i < 16; i++) { a.u32[i] = 0xAAAAAAAAU; b.u32[i] = 0x55555555U; }
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpxord %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0xFFFFFFFFU,
            "VPXORD lane %d: %08x != FFFFFFFF", i, dst.u32[i]);

    /* VPXORD self = 0 */
    for (int i = 0; i < 16; i++) a.u32[i] = (uint32_t)i * 0x11111111U;
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpxord %%zmm0, %%zmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(a) : "zmm0","zmm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u32[i] == 0, "VPXORD self lane %d: %08x", i, dst.u32[i]);

    /* VPANDD with zeroing masking */
    for (int i = 0; i < 16; i++) { a.u32[i] = 0xFFFFFFFFU; b.u32[i] = 0xAAAAAAAAU; }
    uint64_t kmask = 0x00FF;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu32 %1, %%zmm0\n\t"
        "vmovdqu32 %2, %%zmm1\n\t"
        "vpandd %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqu32 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (kmask >> i) & 1 ? 0xAAAAAAAAU : 0;
        TEST_ASSERT(dst.u32[i] == expected,
            "VPANDD zero mask lane %d: %08x != %08x", i, dst.u32[i], expected);
    }

    /* Qword encodings and operand-order-sensitive ANDN reference. */
    for (int i = 0; i < 8; i++) {
        a.u64[i] = UINT64_C(0xf0f0f0f00f0f0f0f) ^ (uint64_t)i;
        b.u64[i] = UINT64_C(0xff00ff0055aa55aa) ^ ((uint64_t)i << 32);
    }
#define TEST_QWORD_BITOP(INSN, EXPR) do { \
        __asm__ volatile ("vmovdqu64 %1, %%zmm0\n\t" "vmovdqu64 %2, %%zmm1\n\t" \
                          INSN " %%zmm1, %%zmm0, %%zmm2\n\t" "vmovdqu64 %%zmm2, %0" \
                          : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"); \
        for (int i = 0; i < 8; i++) { \
            uint64_t expected = (EXPR); \
            TEST_ASSERT(dst.u64[i] == expected, INSN " lane %d: %016llx != %016llx", i, \
                        (unsigned long long)dst.u64[i], (unsigned long long)expected); \
        } \
    } while (0)
    TEST_QWORD_BITOP("vpandq",  a.u64[i] & b.u64[i]);
    TEST_QWORD_BITOP("vpandnq", ~a.u64[i] & b.u64[i]);
    TEST_QWORD_BITOP("vporq",   a.u64[i] | b.u64[i]);
    TEST_QWORD_BITOP("vpxorq",  a.u64[i] ^ b.u64[i]);
#undef TEST_QWORD_BITOP

    /* Empty merge and endpoint-only zero masks. */
    for (int i = 0; i < 8; i++) dst.u64[i] = UINT64_C(0xdeadbeefdeadbeef);
    kmask = 0;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t" "vmovdqu64 %0, %%zmm2\n\t" "vmovdqu64 %1, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t" "vpxorq %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovdqu64 %%zmm2, %0" : "+m"(dst) : "m"(a), "m"(b), "r"(kmask)
        : "zmm0","zmm1","zmm2","k1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u64[i] == UINT64_C(0xdeadbeefdeadbeef), "VPXORQ k=0 merge lane %d", i);

    kmask = UINT64_C(0x81);
    __asm__ volatile (
        "kmovq %3, %%k1\n\t" "vmovdqu64 %1, %%zmm0\n\t" "vmovdqu64 %2, %%zmm1\n\t"
        "vpxorq %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t" "vmovdqu64 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (i == 0 || i == 7) ? (a.u64[i] ^ b.u64[i]) : 0;
        TEST_ASSERT(dst.u64[i] == expected, "VPXORQ endpoint zero mask lane %d", i);
    }

    TEST_END();
}
