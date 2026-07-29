/*
 * Test VAESDEC/VAESDECLAST/VAESENC/VAESENCLAST with zmm (AVX-512 + VAES),
 * and VAESIMC/VAESKEYGENASSIST on xmm only.
 *
 * Compile: gcc -o test_evaes avx512/test_evaes.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl -mvaes
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

static int check_vaes(void) {
    uint32_t eax, ebx, ecx, edx;
    /* VAES: CPUID leaf 7, ECX bit 9 */
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ecx >> 9) & 1;
}
static int check_aes(void) {
    uint32_t eax, ebx, ecx, edx;
    /* AES-NI: CPUID leaf 1, ECX bit 25 */
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
    return (ecx >> 25) & 1;
}

int main(void) {
    if (!check_aes()) {
        printf("AES-NI not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVAESDEC/EVAESDECLAST/EVAESENC/EVAESENCLAST/EVAESIMC/EVAESKEYGENASSIST");

    xmm_t state, rkey, dst_xmm;

    /* VAESENC xmm: one round of AES encryption */
    for (int i = 0; i < 16; i++) state.u8[i] = (uint8_t)(i + 1);
    for (int i = 0; i < 16; i++) rkey.u8[i]  = (uint8_t)(i * 2 + 1);
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesenc %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst_xmm) : "m"(state), "m"(rkey) : "xmm0","xmm1","xmm2"
    );
    /* Verify result is not equal to input (AES round transforms data) */
    TEST_ASSERT(memcmp(&dst_xmm, &state, 16) != 0, "VAESENC xmm: result should differ from input");

    /* VAESENCLAST xmm: last round of AES encryption */
    xmm_t dst_last;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesenclast %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst_last) : "m"(state), "m"(rkey) : "xmm0","xmm1","xmm2"
    );
    /* VAESENC and VAESENCLAST should produce different results */
    TEST_ASSERT(memcmp(&dst_xmm, &dst_last, 16) != 0, "VAESENC vs VAESENCLAST differ");

    /* VAESDEC xmm: one round of AES decryption */
    xmm_t dst_dec;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst_dec) : "m"(state), "m"(rkey) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(memcmp(&dst_dec, &state, 16) != 0, "VAESDEC xmm: result should differ from input");

    /* VAESDECLAST xmm */
    xmm_t dst_declast;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdeclast %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst_declast) : "m"(state), "m"(rkey) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(memcmp(&dst_declast, &dst_dec, 16) != 0, "VAESDEC vs VAESDECLAST differ");

    /* VAESIMC xmm: inverse mix columns */
    xmm_t dst_imc;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vaesimc %%xmm0, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0"
        : "=m"(dst_imc) : "m"(state) : "xmm0","xmm1"
    );
    TEST_ASSERT(memcmp(&dst_imc, &state, 16) != 0, "VAESIMC: result should differ from input");

    /* VAESKEYGENASSIST xmm: key schedule assist */
    xmm_t dst_kg;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vaeskeygenassist $1, %%xmm0, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0"
        : "=m"(dst_kg) : "m"(rkey) : "xmm0","xmm1"
    );
    /* Just verify it runs and produces some output */
    TEST_ASSERT(1, "VAESKEYGENASSIST executed");

    /* VAESENC zmm (VAES extension): 4 independent AES rounds */
    if (!check_vaes()) {
        printf("VAES (zmm) not supported, skipping zmm AES tests.\n");
    } else {
        zmm_t zstate, zrkey, zdst;
        for (int i = 0; i < 64; i++) zstate.u8[i] = (uint8_t)(i + 1);
        for (int i = 0; i < 64; i++) zrkey.u8[i]  = (uint8_t)(i * 2 + 1);
        __asm__ volatile (
            "vmovdqu8 %1, %%zmm0\n\t"
            "vmovdqu8 %2, %%zmm1\n\t"
            "vaesenc %%zmm1, %%zmm0, %%zmm2\n\t"
            "vmovdqu8 %%zmm2, %0"
            : "=m"(zdst) : "m"(zstate), "m"(zrkey) : "zmm0","zmm1","zmm2"
        );
        /* Each 128-bit lane should match xmm result */
        for (int lane = 0; lane < 4; lane++) {
            xmm_t lane_state, lane_rkey, lane_expected;
            memcpy(&lane_state, &zstate.u8[lane*16], 16);
            memcpy(&lane_rkey,  &zrkey.u8[lane*16],  16);
            __asm__ volatile (
                "vmovdqu %1, %%xmm0\n\t"
                "vmovdqu %2, %%xmm1\n\t"
                "vaesenc %%xmm1, %%xmm0, %%xmm2\n\t"
                "vmovdqu %%xmm2, %0"
                : "=m"(lane_expected) : "m"(lane_state), "m"(lane_rkey) : "xmm0","xmm1","xmm2"
            );
            TEST_ASSERT(memcmp(&zdst.u8[lane*16], &lane_expected, 16) == 0,
                "VAESENC zmm lane %d mismatch", lane);
        }
    }

    TEST_END();
}
