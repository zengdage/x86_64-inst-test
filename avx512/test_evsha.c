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

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_sha(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 29) & 1; /* SHA: CPUID.7.EBX[29] */
}
#else
#define check_sha() 1
#endif

int main(void) {
    if (!check_sha()) {
        printf("SHA extensions not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVSHA* (SHA1RNDS4/SHA1MSG1/SHA1MSG2/SHA1NEXTE/SHA256RNDS2/SHA256MSG1/SHA256MSG2 xmm)");

    xmm_t a, b, dst;

    /* SHA1RNDS4: 4 rounds of SHA-1 */
    a = (xmm_t){ .u32 = {0x10325476,0x98BADCFE,0xEFCDAB89,0x67452301} };
    b = (xmm_t){ .u32 = {0x80000000,0,0,0} };
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1rnds4 $0, %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    const xmm_t sha1r0 = { .u32 = {0xf6f86db0,0x38b9e962,0xca055093,0x931064b4} };
    TEST_ASSERT(memcmp(&dst, &sha1r0, 16) == 0, "SHA1RNDS4 round0 golden output");

    /* SHA1MSG1: message schedule */
    a = (xmm_t){ .u32 = {0x61626364,0x65666768,0x696A6B6C,0x6D6E6F70} };
    b = (xmm_t){ .u32 = {0x71727374,0x75767778,0x797A3031,0x32333435} };
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1msg1 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    const xmm_t sha1m1 = { .u32 = {0x18185355,0x5755535d,0x08080808,0x08080818} };
    TEST_ASSERT(memcmp(&dst, &sha1m1, 16) == 0, "SHA1MSG1 golden output");

    /* SHA1MSG2: message schedule step 2 */
    a = (xmm_t){ .u32 = {0x12345678,0x9ABCDEF0,0x0FEDCBA9,0x87654321} };
    b = (xmm_t){ .u32 = {0xAAAAAAAA,0xBBBBBBBB,0xCCCCCCCC,0xDDDDDDDD} };
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1msg2 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    const xmm_t sha1m2 = { .u32 = {0x0ace9345,0x602ce8b4,0x68ace025,0x97531fda} };
    TEST_ASSERT(memcmp(&dst, &sha1m2, 16) == 0, "SHA1MSG2 golden output");

    /* SHA1NEXTE: add e to state */
    a = (xmm_t){ .u32 = {0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476} };
    b = (xmm_t){ .u32 = {0xAAAAAAAA,0xBBBBBBBB,0xCCCCCCCC,0xDDDDDDDD} };
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha1nexte %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    const xmm_t sha1ne = { .u32 = {0xaaaaaaaa,0xbbbbbbbb,0xcccccccc,0x61ea72fa} };
    TEST_ASSERT(memcmp(&dst, &sha1ne, 16) == 0, "SHA1NEXTE golden output");

    /* SHA256RNDS2: 2 rounds of SHA-256 */
    a = (xmm_t){ .u32 = {0x5be0cd19,0x1f83d9ab,0x9b05688c,0x510e527f} };
    b = (xmm_t){ .u32 = {0xe9b5dba5,0xb5c0fbcf,0x71374491,0x6a09e667} };
    xmm_t wk = { .u32 = {0x5a827999,0x6ed9eba1,0,0} };
    __asm__ volatile (
        "vmovdqu %1, %%xmm1\n\t"
        "vmovdqu %2, %%xmm2\n\t"
        "vmovdqu %3, %%xmm0\n\t"
        "sha256rnds2 %%xmm2, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0"
        : "=m"(dst) : "m"(a), "m"(b), "m"(wk) : "xmm0","xmm1","xmm2"
    );
    const xmm_t sha256r = { .u32 = {0xa448e70d,0xc0b83fca,0x48737976,0x6c2896a9} };
    TEST_ASSERT(memcmp(&dst, &sha256r, 16) == 0, "SHA256RNDS2 golden output");

    /* SHA256MSG1: message schedule */
    a = (xmm_t){ .u32 = {0x61626380,0x12345678,0x9ABCDEF0,0xCAFEBABE} };
    b = (xmm_t){ .u32 = {0xDEADBEEF,0x11111111,0x22222222,0x33333333} };
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha256msg1 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    const xmm_t sha256m1 = { .u32 = {0x495f4a6e,0xd8131b44,0x6522778d,0x76d1d5c9} };
    TEST_ASSERT(memcmp(&dst, &sha256m1, 16) == 0, "SHA256MSG1 golden output");

    /* SHA256MSG2: message schedule step 2 */
    a = (xmm_t){ .u32 = {0x12345678,0x9ABCDEF0,0x0FEDCBA9,0x87654321} };
    b = (xmm_t){ .u32 = {0xAAAAAAAA,0xBBBBBBBB,0xCCCCCCCC,0xDDDDDDDD} };
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t"
        "vmovdqu %2, %%xmm1\n\t"
        "sha256msg2 %%xmm1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    const xmm_t sha256m2 = { .u32 = {0x12012344,0xf01f0112,0xc5bbd6b1,0xe7fc64ed} };
    TEST_ASSERT(memcmp(&dst, &sha256m2, 16) == 0, "SHA256MSG2 golden output");

    /* Verify the remaining SHA1RNDS4 function selectors. */
    a = (xmm_t){ .u32 = {0x10325476,0x98BADCFE,0xEFCDAB89,0x67452301} };
    b = (xmm_t){ .u32 = {0x80000000,0,0,0} };
    xmm_t r1, r2, r3;
    __asm__ volatile (
        "vmovdqu %3,%%xmm0\n\tvmovdqu %4,%%xmm1\n\tvmovdqa %%xmm0,%%xmm2\n\tvmovdqa %%xmm0,%%xmm3\n\t"
        "sha1rnds4 $1,%%xmm1,%%xmm0\n\tsha1rnds4 $2,%%xmm1,%%xmm2\n\tsha1rnds4 $3,%%xmm1,%%xmm3\n\t"
        "vmovdqu %%xmm0,%0\n\tvmovdqu %%xmm2,%1\n\tvmovdqu %%xmm3,%2"
        : "=m"(r1), "=m"(r2), "=m"(r3) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2", "xmm3");
    const xmm_t e1 = { .u32 = {0xafb0dbb3,0xf6e1abc2,0x154bf706,0x423373bd} };
    const xmm_t e2 = { .u32 = {0x841ebe81,0x8aa82f3d,0xd5e1a361,0x4fd450b8} };
    const xmm_t e3 = { .u32 = {0xc6931140,0x2a0a92f3,0xa0d57f8a,0x186e3058} };
    TEST_ASSERT(memcmp(&r1,&e1,16)==0 && memcmp(&r2,&e2,16)==0 && memcmp(&r3,&e3,16)==0,
        "SHA1RNDS4 functions 1/2/3 golden outputs");

    TEST_END();
}
