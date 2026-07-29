#include "../common.h"
#include <immintrin.h>

static int check_gfni(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return (ecx >> 8) & 1; /* GFNI bit */
}

int main(void) {
    TEST_START("VGF2P8MULB/VGF2P8AFFINEQB/VGF2P8AFFINEINVQB 256-bit VEX");

    if (!check_gfni()) {
        printf("GFNI not supported, skipping\n");
        return 0;
    }

    /* VGF2P8MULB: GF(2^8) multiply with irreducible polynomial x^8+x^4+x^3+x+1
       Known: 0x02 * 0x87 = 0x15 (standard AES GF multiplication) */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 32; i++) { a.u8[i] = 0x02; b.u8[i] = 0x87; }
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovdqu %2, %%ymm1\n\t"
            "vgf2p8mulb %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        int ok = 1;
        for (int i = 0; i < 32; i++) if (r.u8[i] != 0x15) ok = 0;
        TEST_ASSERT(ok, "vgf2p8mulb 0x02*0x87=0x15 (all 32 bytes)");
    }

    /* VGF2P8MULB: multiply by 1 => identity */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 32; i++) { a.u8[i] = (uint8_t)i; b.u8[i] = 0x01; }
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovdqu %2, %%ymm1\n\t"
            "vgf2p8mulb %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        int ok = 1;
        for (int i = 0; i < 32; i++) if (r.u8[i] != (uint8_t)i) ok = 0;
        TEST_ASSERT(ok, "vgf2p8mulb x*1=x identity");
    }

    /* VGF2P8MULB: multiply by 0 => 0 */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 32; i++) { a.u8[i] = 0xFF; b.u8[i] = 0x00; }
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovdqu %2, %%ymm1\n\t"
            "vgf2p8mulb %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        int ok = 1;
        for (int i = 0; i < 32; i++) if (r.u8[i] != 0) ok = 0;
        TEST_ASSERT(ok, "vgf2p8mulb x*0=0");
    }

    /* VGF2P8AFFINEQB: affine transformation over GF(2)
       result[i] = popcount(matrix_row[i] AND src_byte) XOR imm8_bit[i]
       With identity matrix (each row = 1<<(7-i)) and imm8=0, should be identity */
    {
        ymm_t src, matrix, r;
        /* identity matrix in each qword: row i selects bit (7-i) */
        uint64_t identity = 0x0102040810204080ULL;
        for (int i = 0; i < 4; i++) matrix.u64[i] = identity;
        for (int i = 0; i < 32; i++) src.u8[i] = (uint8_t)(i + 0x40);
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovdqu %2, %%ymm1\n\t"
            "vgf2p8affineqb $0, %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(r) : "m"(src), "m"(matrix) : "ymm0","ymm1","ymm2");
        int ok = 1;
        for (int i = 0; i < 32; i++) if (r.u8[i] != (uint8_t)(i + 0x40)) ok = 0;
        TEST_ASSERT(ok, "vgf2p8affineqb identity matrix imm=0");
    }

    /* VGF2P8AFFINEQB with imm8=0xFF => bitwise NOT of identity result */
    {
        ymm_t src, matrix, r;
        uint64_t identity = 0x0102040810204080ULL;
        for (int i = 0; i < 4; i++) matrix.u64[i] = identity;
        for (int i = 0; i < 32; i++) src.u8[i] = 0x00;
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovdqu %2, %%ymm1\n\t"
            "vgf2p8affineqb $0xFF, %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(r) : "m"(src), "m"(matrix) : "ymm0","ymm1","ymm2");
        int ok = 1;
        for (int i = 0; i < 32; i++) if (r.u8[i] != 0xFF) ok = 0;
        TEST_ASSERT(ok, "vgf2p8affineqb identity(0) XOR 0xFF = 0xFF");
    }

    /* VGF2P8AFFINEINVQB: affine transformation of GF(2^8) multiplicative inverse
       With identity matrix and imm8=0, result = GF inverse of each byte
       Inverse of 1 is 1, inverse of 0 is 0 (by convention) */
    {
        ymm_t src, matrix, r;
        uint64_t identity = 0x0102040810204080ULL;
        for (int i = 0; i < 4; i++) matrix.u64[i] = identity;
        for (int i = 0; i < 32; i++) src.u8[i] = 0x01;
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovdqu %2, %%ymm1\n\t"
            "vgf2p8affineinvqb $0, %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(r) : "m"(src), "m"(matrix) : "ymm0","ymm1","ymm2");
        int ok = 1;
        for (int i = 0; i < 32; i++) if (r.u8[i] != 0x01) ok = 0;
        TEST_ASSERT(ok, "vgf2p8affineinvqb inv(1)=1");
    }

    /* VGF2P8AFFINEINVQB: inverse of 0 is 0 */
    {
        ymm_t src, matrix, r;
        uint64_t identity = 0x0102040810204080ULL;
        for (int i = 0; i < 4; i++) matrix.u64[i] = identity;
        for (int i = 0; i < 32; i++) src.u8[i] = 0x00;
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovdqu %2, %%ymm1\n\t"
            "vgf2p8affineinvqb $0, %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovdqu %%ymm2, %0"
            : "=m"(r) : "m"(src), "m"(matrix) : "ymm0","ymm1","ymm2");
        int ok = 1;
        for (int i = 0; i < 32; i++) if (r.u8[i] != 0x00) ok = 0;
        TEST_ASSERT(ok, "vgf2p8affineinvqb inv(0)=0");
    }

    TEST_END();
}
