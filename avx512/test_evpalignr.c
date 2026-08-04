/*
 * Test VPALIGNR with zmm registers (AVX-512BW).
 * Packed align right: concatenate two zmm registers and extract 64-byte window.
 *
 * Compile: gcc -o test_evpalignr avx512/test_evpalignr.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

#define RUN_ALIGN(imm, lhs, rhs, out) do { \
    __asm__ volatile("vmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvpalignr $" #imm ",%%zmm1,%%zmm0,%%zmm2\n\tvmovdqu8 %%zmm2,%0" \
        : "=m"(out) : "m"(lhs), "m"(rhs) : "zmm0", "zmm1", "zmm2"); \
} while (0)

static uint8_t align_expected(const zmm_t *a, const zmm_t *b, int lane, int pos, unsigned count) {
    unsigned index = count + (unsigned)pos;
    if (index < 16) return b->u8[lane * 16 + (int)index];
    if (index < 32) return a->u8[lane * 16 + (int)index - 16];
    return 0;
}

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512bw(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}
#else
#define check_avx512bw() 1
#endif

int main(void) {
    if (!check_avx512bw()) {
        printf("AVX-512BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPALIGNR (VPALIGNR zmm)");

    zmm_t a, b, dst;

    /*
     * VPALIGNR zmm: operates per 16-byte lane.
     * Within each 16-byte lane: concatenate b_lane:a_lane (32 bytes),
     * then extract bytes [imm8..imm8+15].
     */

    /*
     * AT&T: vpalignr $N, zmm1, zmm0, zmm2
     * Intel: VPALIGNR zmm2, zmm0, zmm1, N
     * Concatenates zmm0:zmm1 (zmm0=high, zmm1=low), shifts right N bytes per lane.
     * imm=0  → result = zmm1 (low = b)
     * imm=16 → result = zmm0 (high = a)
     * So load zmm0=a (high src), zmm1=b (low src).
     */

    /* imm8=0: result = b (low src) */
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)(i + 1); b.u8[i] = (uint8_t)(i + 100); }
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpalignr $0, %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    TEST_ASSERT(memcmp(&dst, &b, 64) == 0, "VPALIGNR imm=0 should equal b (low src)");

    /* imm8=16: result = a (high src) */
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpalignr $16, %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    TEST_ASSERT(memcmp(&dst, &a, 64) == 0, "VPALIGNR imm=16 should equal a (high src)");

    /*
     * imm8=4: per lane, concat a_lane:b_lane (32 bytes), extract [4..19].
     * bytes [0..11] = b_lane[4..15], bytes [12..15] = a_lane[0..3]
     */
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpalignr $4, %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 12; j++)
            TEST_ASSERT(dst.u8[lane*16+j] == b.u8[lane*16+4+j],
                "VPALIGNR imm=4 lane%d j%d: %u != %u", lane, j,
                dst.u8[lane*16+j], b.u8[lane*16+4+j]);
        for (int j = 0; j < 4; j++)
            TEST_ASSERT(dst.u8[lane*16+12+j] == a.u8[lane*16+j],
                "VPALIGNR imm=4 lane%d b%d: %u != %u", lane, j,
                dst.u8[lane*16+12+j], a.u8[lane*16+j]);
    }

    /* imm8=8: bytes [0..7]=b_lane[8..15], bytes [8..15]=a_lane[0..7] */
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpalignr $8, %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++) {
        for (int j = 0; j < 8; j++)
            TEST_ASSERT(dst.u8[lane*16+j] == b.u8[lane*16+8+j],
                "VPALIGNR imm=8 lane%d b%d: %u != %u", lane, j,
                dst.u8[lane*16+j], b.u8[lane*16+8+j]);
        for (int j = 0; j < 8; j++)
            TEST_ASSERT(dst.u8[lane*16+8+j] == a.u8[lane*16+j],
                "VPALIGNR imm=8 lane%d a%d: %u != %u", lane, j,
                dst.u8[lane*16+8+j], a.u8[lane*16+j]);
    }

    /* VPALIGNR with zeroing masking: upper 32 bytes zeroed */
    uint64_t kmask = 0x00000000FFFFFFFFULL;
    __asm__ volatile (
        "kmovq %3, %%k1\n\t"
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpalignr $4, %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(a), "m"(b), "r"(kmask) : "zmm0","zmm1","zmm2","k1"
    );
    for (int i = 32; i < 64; i++)
        TEST_ASSERT(dst.u8[i] == 0, "VPALIGNR zero mask upper byte %d: %u", i, dst.u8[i]);

#define CHECK_ALIGN(imm) do { \
    RUN_ALIGN(imm, a, b, dst); \
    for (int lane = 0; lane < 4; lane++) for (int j = 0; j < 16; j++) \
        TEST_ASSERT(dst.u8[lane*16+j] == align_expected(&a, &b, lane, j, imm), "VPALIGNR imm=%d lane%d byte%d", imm, lane, j); \
} while (0)
    CHECK_ALIGN(15);
    CHECK_ALIGN(17);
    CHECK_ALIGN(31);
    CHECK_ALIGN(32);
    CHECK_ALIGN(255);
#undef CHECK_ALIGN

    zmm_t initial; memset(&initial, 0x5a, sizeof(initial));
    kmask = 0;
    __asm__ volatile ("kmovq %4,%%k1\n\tvmovdqu8 %1,%%zmm0\n\tvmovdqu8 %2,%%zmm1\n\tvmovdqu8 %3,%%zmm2\n\tvpalignr $15,%%zmm1,%%zmm0,%%zmm2%{%%k1%}\n\tvmovdqu8 %%zmm2,%0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(initial), "r"(kmask) : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 64; i++) TEST_ASSERT(dst.u8[i] == 0x5a, "VPALIGNR k=0 merge byte %d", i);

    TEST_END();
}
