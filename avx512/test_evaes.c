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
#include "../crypto/aes_ref.h"

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
static int check_vaes(void) {
    uint32_t eax, ebx, ecx, edx;
    /* VAES: CPUID leaf 7, ECX bit 9 */
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ecx >> 9) & 1;
}
#else
#define check_vaes() 1
#endif
#if ENABLE_RUNTIME_CPU_CHECKS
static int check_aes(void) {
    uint32_t eax, ebx, ecx, edx;
    /* AES-NI: CPUID leaf 1, ECX bit 25 */
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
    return (ecx >> 25) & 1;
}
#else
#define check_aes() 1
#endif

int main(void) {
    if (!check_aes()) {
        printf("AES-NI not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVAESDEC/EVAESDECLAST/EVAESENC/EVAESENCLAST/EVAESIMC/EVAESKEYGENASSIST");

    xmm_t state, rkey, dst_xmm, expected;

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
    aes_ref_enc_round(state.u8, rkey.u8, expected.u8, 0);
    TEST_ASSERT(memcmp(&dst_xmm, &expected, 16) == 0,
                "VAESENC xmm matches independent reference");

    /* VAESENCLAST xmm: last round of AES encryption */
    xmm_t dst_last;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesenclast %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst_last) : "m"(state), "m"(rkey) : "xmm0","xmm1","xmm2"
    );
    aes_ref_enc_round(state.u8, rkey.u8, expected.u8, 1);
    TEST_ASSERT(memcmp(&dst_last, &expected, 16) == 0,
                "VAESENCLAST xmm matches independent reference");

    /* VAESDEC xmm: one round of AES decryption */
    xmm_t dst_dec;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdec %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst_dec) : "m"(state), "m"(rkey) : "xmm0","xmm1","xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 0);
    TEST_ASSERT(memcmp(&dst_dec, &expected, 16) == 0,
                "VAESDEC xmm matches independent reference");

    /* VAESDECLAST xmm */
    xmm_t dst_declast;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vaesdeclast %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(dst_declast) : "m"(state), "m"(rkey) : "xmm0","xmm1","xmm2"
    );
    aes_ref_dec_round(state.u8, rkey.u8, expected.u8, 1);
    TEST_ASSERT(memcmp(&dst_declast, &expected, 16) == 0,
                "VAESDECLAST xmm matches independent reference");

    /* VAESIMC xmm: inverse mix columns */
    xmm_t dst_imc;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vaesimc %%xmm0, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0"
        : "=m"(dst_imc) : "m"(state) : "xmm0","xmm1"
    );
    aes_ref_inv_mix(state.u8, expected.u8);
    TEST_ASSERT(memcmp(&dst_imc, &expected, 16) == 0,
                "VAESIMC matches independent reference");

    /* VAESKEYGENASSIST xmm: key schedule assist */
    xmm_t dst_kg;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vaeskeygenassist $1, %%xmm0, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0"
        : "=m"(dst_kg) : "m"(rkey) : "xmm0","xmm1"
    );
    aes_ref_keygenassist(rkey.u8, 1, expected.u8);
    TEST_ASSERT(memcmp(&dst_kg, &expected, 16) == 0,
                "VAESKEYGENASSIST matches independent reference");

    /* VAESENC zmm (VAES extension): 4 independent AES rounds */
    if (!check_vaes()) {
        printf("VAES (zmm) not supported, skipping zmm AES tests.\n");
    } else {
        zmm_t zstate, zrkey, zenc, zlast, zdec, zdeclast;
        for (int i = 0; i < 64; i++) zstate.u8[i] = (uint8_t)(i + 1);
        for (int i = 0; i < 64; i++) zrkey.u8[i]  = (uint8_t)(i * 2 + 1);
        __asm__ volatile (
            "vmovdqu8 %4, %%zmm0\n\t"
            "vmovdqu8 %5, %%zmm1\n\t"
            "vaesenc %%zmm1, %%zmm0, %%zmm2\n\t"
            "vaesenclast %%zmm1, %%zmm0, %%zmm3\n\t"
            "vaesdec %%zmm1, %%zmm0, %%zmm4\n\t"
            "vaesdeclast %%zmm1, %%zmm0, %%zmm5\n\t"
            "vmovdqu8 %%zmm2, %0\n\t"
            "vmovdqu8 %%zmm3, %1\n\t"
            "vmovdqu8 %%zmm4, %2\n\t"
            "vmovdqu8 %%zmm5, %3"
            : "=m"(zenc), "=m"(zlast), "=m"(zdec), "=m"(zdeclast)
            : "m"(zstate), "m"(zrkey)
            : "zmm0","zmm1","zmm2","zmm3","zmm4","zmm5"
        );
        /* Each instruction operates independently in every 128-bit lane. */
        for (int lane = 0; lane < 4; lane++) {
            xmm_t lane_state, lane_rkey, lane_expected;
            memcpy(&lane_state, &zstate.u8[lane*16], 16);
            memcpy(&lane_rkey,  &zrkey.u8[lane*16],  16);
            aes_ref_enc_round(lane_state.u8, lane_rkey.u8, lane_expected.u8, 0);
            TEST_ASSERT(memcmp(&zenc.u8[lane*16], &lane_expected, 16) == 0,
                "VAESENC zmm lane %d mismatch", lane);
            aes_ref_enc_round(lane_state.u8, lane_rkey.u8, lane_expected.u8, 1);
            TEST_ASSERT(memcmp(&zlast.u8[lane*16], &lane_expected, 16) == 0,
                "VAESENCLAST zmm lane %d mismatch", lane);
            aes_ref_dec_round(lane_state.u8, lane_rkey.u8, lane_expected.u8, 0);
            TEST_ASSERT(memcmp(&zdec.u8[lane*16], &lane_expected, 16) == 0,
                "VAESDEC zmm lane %d mismatch", lane);
            aes_ref_dec_round(lane_state.u8, lane_rkey.u8, lane_expected.u8, 1);
            TEST_ASSERT(memcmp(&zdeclast.u8[lane*16], &lane_expected, 16) == 0,
                "VAESDECLAST zmm lane %d mismatch", lane);
        }
    }

    TEST_END();
}
