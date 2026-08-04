/*
 * Test VPINSRB/VPINSRD/VPINSRQ/VPINSRW with xmm registers (AVX-512VL).
 * Insert integer element into xmm register.
 *
 * Compile: gcc -o test_evpins avx512/test_evpins.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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
static int check_avx512vl(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 31) & 1); /* AVX512F + AVX512VL */
}
#else
#define check_avx512vl() 1
#endif

int main(void) {
    if (!check_avx512vl()) {
        printf("AVX-512VL not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPINSRB/EVPINSRW/EVPINSRD/EVPINSRQ (xmm AVX-512VL)");

    xmm_t dst;

    /* VPINSRB: insert byte at position 0..15 */
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vpxor %%xmm0, %%xmm0, %%xmm0\n\t"
        "vpinsrb $3, %1, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "r"((int)0xAB) : "xmm0"
    );
    TEST_ASSERT(dst.u8[3] == 0xAB, "VPINSRB pos3: %02x", dst.u8[3]);
    TEST_ASSERT(dst.u8[0] == 0, "VPINSRB pos0 unchanged: %02x", dst.u8[0]);

    /* VPINSRB: insert at position 15 */
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vpxor %%xmm0, %%xmm0, %%xmm0\n\t"
        "vpinsrb $15, %1, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "r"((int)0xFF) : "xmm0"
    );
    TEST_ASSERT(dst.u8[15] == 0xFF, "VPINSRB pos15: %02x", dst.u8[15]);

    /* VPINSRW: insert word at position 0..7 */
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vpxor %%xmm0, %%xmm0, %%xmm0\n\t"
        "vpinsrw $2, %1, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "r"((int)0x1234) : "xmm0"
    );
    TEST_ASSERT(dst.u16[2] == 0x1234, "VPINSRW pos2: %04x", dst.u16[2]);

    /* VPINSRD: insert dword at position 0..3 */
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vpxor %%xmm0, %%xmm0, %%xmm0\n\t"
        "vpinsrd $1, %1, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "r"((int)0xDEADBEEF) : "xmm0"
    );
    TEST_ASSERT(dst.u32[1] == 0xDEADBEEFU, "VPINSRD pos1: %08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[0] == 0, "VPINSRD pos0 unchanged: %08x", dst.u32[0]);

    /* VPINSRQ: insert qword at position 0..1 */
    memset(&dst, 0, sizeof(dst));
    uint64_t qval = 0xCAFEBABEDEADBEEFULL;
    __asm__ volatile (
        "vpxor %%xmm0, %%xmm0, %%xmm0\n\t"
        "vpinsrq $1, %1, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "r"(qval) : "xmm0"
    );
    TEST_ASSERT(dst.u64[1] == qval, "VPINSRQ pos1: %016llx", (unsigned long long)dst.u64[1]);
    TEST_ASSERT(dst.u64[0] == 0, "VPINSRQ pos0 unchanged: %016llx", (unsigned long long)dst.u64[0]);

    /* VPINSRD: insert into non-zero register (preserve other lanes) */
    memset(&dst, 0xFF, sizeof(dst));
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpinsrd $2, %2, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(dst), "r"((int)0x12345678) : "xmm0"
    );
    TEST_ASSERT(dst.u32[2] == 0x12345678U, "VPINSRD preserve pos2: %08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFFU, "VPINSRD preserve pos0: %08x", dst.u32[0]);

    /* Boundary: insert 0 */
    memset(&dst, 0xFF, sizeof(dst));
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vpinsrd $0, %2, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(dst), "r"((int)0) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0, "VPINSRD insert-0 pos0: %08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0xFFFFFFFFU, "VPINSRD insert-0 pos1 unchanged: %08x", dst.u32[1]);

    /* Immediate high bits are ignored; low bits select the final element. */
    xmm_t original;
    for (int i = 0; i < 16; i++) original.u8[i] = (uint8_t)i;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t" "vpinsrb $0xff, %2, %%xmm0, %%xmm1\n\t" "vmovdqu %%xmm1, %0"
        : "=m"(dst) : "m"(original), "r"((int)0xa5) : "xmm0","xmm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst.u8[i] == (i == 15 ? UINT8_C(0xa5) : original.u8[i]),
                    "VPINSRB imm=ff lane %d", i);

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t" "vpinsrw $0xff, %2, %%xmm0, %%xmm1\n\t" "vmovdqu %%xmm1, %0"
        : "=m"(dst) : "m"(original), "r"((int)0xbeef) : "xmm0","xmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u16[i] == (i == 7 ? UINT16_C(0xbeef) : original.u16[i]),
                    "VPINSRW imm=ff lane %d", i);

    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t" "vpinsrd $0xff, %2, %%xmm0, %%xmm1\n\t" "vmovdqu %%xmm1, %0"
        : "=m"(dst) : "m"(original), "r"((int)UINT32_C(0x89abcdef)) : "xmm0","xmm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst.u32[i] == (i == 3 ? UINT32_C(0x89abcdef) : original.u32[i]),
                    "VPINSRD imm=ff lane %d", i);

    qval = UINT64_C(0x0123456789abcdef);
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t" "vpinsrq $0xff, %2, %%xmm0, %%xmm1\n\t" "vmovdqu %%xmm1, %0"
        : "=m"(dst) : "m"(original), "r"(qval) : "xmm0","xmm1");
    TEST_ASSERT(dst.u64[0] == original.u64[0] && dst.u64[1] == qval,
                "VPINSRQ imm=ff selects qword 1 and preserves qword 0");

    TEST_END();
}
