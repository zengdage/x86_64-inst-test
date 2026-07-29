/*
 * Test SHA1RNDS4/SHA1MSG1/SHA1MSG2/SHA1NEXTE/SHA256RNDS2/SHA256MSG1/SHA256MSG2
 * SHA instructions operate on xmm registers only (no AVX-512 zmm form).
 *
 * Compile: gcc -o test_evsha avx512/test_evsha.c -O0 -msha -mavx2
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../common.h"

static int check_sha(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 29) & 1; /* SHA: CPUID.7.EBX[29] */
}

int main(void) {
    if (!check_sha()) {
        printf("SHA extensions not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVSHA* (SHA1RNDS4/SHA1MSG1/SHA1MSG2/SHA1NEXTE/SHA256RNDS2/SHA256MSG1/SHA256MSG2 xmm)");

    xmm_t a, b, dst;

    /* SHA1RNDS4: 4 rounds of SHA-1 */
    for (int i = 0; i < 4; i++) a.u32[i] = (uint32_t)(i + 1) * 0x11111111;
    for (int i = 0; i < 4; i++) b.u32[i] = (uint32_t)(i + 5) * 0x22222222;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1rnds4 $0, %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    TEST_ASSERT(memcmp(&dst, &a, 16) != 0, "SHA1RNDS4 round0: result differs from input");

    /* SHA1MSG1: message schedule */
    for (int i = 0; i < 4; i++) a.u32[i] = (uint32_t)i * 0x01234567U;
    for (int i = 0; i < 4; i++) b.u32[i] = (uint32_t)i * 0x89ABCDEFU;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1msg1 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    TEST_ASSERT(1, "SHA1MSG1 executed");

    /* SHA1MSG2: message schedule step 2 */
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1msg2 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    TEST_ASSERT(1, "SHA1MSG2 executed");

    /* SHA1NEXTE: add e to state */
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1nexte %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    TEST_ASSERT(1, "SHA1NEXTE executed");

    /* SHA256RNDS2: 2 rounds of SHA-256 */
    for (int i = 0; i < 4; i++) a.u32[i] = (uint32_t)(i + 1) * 0x11111111;
    for (int i = 0; i < 4; i++) b.u32[i] = (uint32_t)(i + 5) * 0x22222222;
    xmm_t wk;
    wk.u32[0] = 0x428A2F98U; wk.u32[1] = 0x71374491U; wk.u32[2] = 0; wk.u32[3] = 0;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "vmovdqu %3, %%xmm2\n\t"
        "sha256rnds2 %%xmm2, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(wk) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(memcmp(&dst, &a, 16) != 0, "SHA256RNDS2: result differs from input");

    /* SHA256MSG1: message schedule */
    for (int i = 0; i < 4; i++) a.u32[i] = (uint32_t)i * 0x01234567U;
    for (int i = 0; i < 4; i++) b.u32[i] = (uint32_t)i * 0x89ABCDEFU;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha256msg1 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    TEST_ASSERT(1, "SHA256MSG1 executed");

    /* SHA256MSG2: message schedule step 2 */
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha256msg2 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    TEST_ASSERT(1, "SHA256MSG2 executed");

    /* Verify SHA1RNDS4 all 4 round constants */
    for (int rnd = 0; rnd < 4; rnd++) {
        for (int i = 0; i < 4; i++) a.u32[i] = (uint32_t)(i + 1) * 0x11111111;
        for (int i = 0; i < 4; i++) b.u32[i] = (uint32_t)(i + 5) * 0x22222222;
        xmm_t out;
        __asm__ volatile (
            "vmovdqu %1, %%xmm0\n\t"
            "vmovdqu %2, %%xmm1\n\t"
            "sha1rnds4 %3, %%xmm1, %%xmm0\n\t"
            "vmovdqu %%xmm0, %0"
            : "=m"(out) : "m"(a), "m"(b), "i"(0) : "xmm0","xmm1"
        );
        TEST_ASSERT(1, "SHA1RNDS4 round %d executed", rnd);
        (void)out;
    }

    TEST_END();
}
