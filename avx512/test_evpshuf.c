/*
 * Test VPSHUFB/VPSHUFD/VPSHUFHW/VPSHUFLW with zmm registers (AVX-512BW/F).
 * Packed shuffle byte/dword/high-word/low-word.
 *
 * Compile: gcc -o test_evpshuf avx512/test_evpshuf.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
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

static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return ((ebx >> 16) & 1) && ((ebx >> 30) & 1);
}

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512F/BW not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVPSHUFB/EVPSHUFD/EVPSHUFHW/EVPSHUFLW (zmm)");

    zmm_t src, ctrl, dst;

    /* VPSHUFD: shuffle dwords with imm8=0b00011011 (reverse each group of 4) */
    for (int i = 0; i < 16; i++) src.u32[i] = (uint32_t)(i + 1);
    __asm__ volatile (
        "vmovdqu32 %1, %%zmm0\n\t"
        "vpshufd $0x1B, %%zmm0, %%zmm1\n\t"  /* 0x1B = 0b00011011: 3,2,1,0 */
        "vmovdqu32 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    /* Each group of 4 dwords reversed: [0..3]->[3,2,1,0], [4..7]->[7,6,5,4], etc. */
    for (int g = 0; g < 4; g++) {
        TEST_ASSERT(dst.u32[g*4+0] == src.u32[g*4+3], "VPSHUFD g%d lane0: %u", g, dst.u32[g*4+0]);
        TEST_ASSERT(dst.u32[g*4+1] == src.u32[g*4+2], "VPSHUFD g%d lane1: %u", g, dst.u32[g*4+1]);
        TEST_ASSERT(dst.u32[g*4+2] == src.u32[g*4+1], "VPSHUFD g%d lane2: %u", g, dst.u32[g*4+2]);
        TEST_ASSERT(dst.u32[g*4+3] == src.u32[g*4+0], "VPSHUFD g%d lane3: %u", g, dst.u32[g*4+3]);
    }

    /* VPSHUFHW: shuffle high words with imm8=0b00011011 */
    for (int i = 0; i < 32; i++) src.u16[i] = (uint16_t)(i + 1);
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vpshufhw $0x1B, %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    /* Low 4 words of each 8-word group unchanged; high 4 words shuffled */
    for (int g = 0; g < 4; g++) {
        for (int j = 0; j < 4; j++)
            TEST_ASSERT(dst.u16[g*8+j] == src.u16[g*8+j], "VPSHUFHW low g%d j%d: %u", g, j, dst.u16[g*8+j]);
        TEST_ASSERT(dst.u16[g*8+4] == src.u16[g*8+7], "VPSHUFHW high g%d pos0: %u", g, dst.u16[g*8+4]);
        TEST_ASSERT(dst.u16[g*8+5] == src.u16[g*8+6], "VPSHUFHW high g%d pos1: %u", g, dst.u16[g*8+5]);
        TEST_ASSERT(dst.u16[g*8+6] == src.u16[g*8+5], "VPSHUFHW high g%d pos2: %u", g, dst.u16[g*8+6]);
        TEST_ASSERT(dst.u16[g*8+7] == src.u16[g*8+4], "VPSHUFHW high g%d pos3: %u", g, dst.u16[g*8+7]);
    }

    /* VPSHUFLW: shuffle low words with imm8=0b00011011 */
    __asm__ volatile (
        "vmovdqu16 %1, %%zmm0\n\t"
        "vpshuflw $0x1B, %%zmm0, %%zmm1\n\t"
        "vmovdqu16 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm0","zmm1"
    );
    for (int g = 0; g < 4; g++) {
        TEST_ASSERT(dst.u16[g*8+0] == src.u16[g*8+3], "VPSHUFLW low g%d pos0: %u", g, dst.u16[g*8+0]);
        TEST_ASSERT(dst.u16[g*8+1] == src.u16[g*8+2], "VPSHUFLW low g%d pos1: %u", g, dst.u16[g*8+1]);
        TEST_ASSERT(dst.u16[g*8+2] == src.u16[g*8+1], "VPSHUFLW low g%d pos2: %u", g, dst.u16[g*8+2]);
        TEST_ASSERT(dst.u16[g*8+3] == src.u16[g*8+0], "VPSHUFLW low g%d pos3: %u", g, dst.u16[g*8+3]);
        for (int j = 4; j < 8; j++)
            TEST_ASSERT(dst.u16[g*8+j] == src.u16[g*8+j], "VPSHUFLW high g%d j%d: %u", g, j, dst.u16[g*8+j]);
    }

    /* VPSHUFB: byte shuffle within each 16-byte lane */
    for (int i = 0; i < 64; i++) src.u8[i] = (uint8_t)(i + 1);
    /* ctrl: reverse bytes within each 16-byte lane */
    for (int lane = 0; lane < 4; lane++)
        for (int j = 0; j < 16; j++)
            ctrl.u8[lane*16+j] = (uint8_t)(15 - j);
    __asm__ volatile (
        "vmovdqu8 %1, %%zmm0\n\t"
        "vmovdqu8 %2, %%zmm1\n\t"
        "vpshufb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqu8 %%zmm2, %0"
        : "=m"(dst) : "m"(src), "m"(ctrl) : "zmm0","zmm1","zmm2"
    );
    for (int lane = 0; lane < 4; lane++)
        for (int j = 0; j < 16; j++)
            TEST_ASSERT(dst.u8[lane*16+j] == src.u8[lane*16 + (15-j)],
                "VPSHUFB lane%d byte%d: %u != %u", lane, j,
                dst.u8[lane*16+j], src.u8[lane*16+(15-j)]);

    TEST_END();
}
