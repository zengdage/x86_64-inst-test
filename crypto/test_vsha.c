#include "../common.h"
#include <immintrin.h>

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_sha(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return (ebx >> 29) & 1; /* SHA bit */
}
#else
#define check_sha() 1
#endif

static void test_sha_golden_outputs(void) {
    xmm_t state = { .u32 = {0x10325476,0x98BADCFE,0xEFCDAB89,0x67452301} };
    xmm_t msg = { .u32 = {0x80000000,0,0,0} }, r0, r1, r2, r3;
    __asm__ volatile(
        "movdqu %4,%%xmm0\n\tmovdqu %5,%%xmm1\n\t"
        "movdqa %%xmm0,%%xmm2\n\tmovdqa %%xmm0,%%xmm3\n\tmovdqa %%xmm0,%%xmm4\n\t"
        "sha1rnds4 $0,%%xmm1,%%xmm0\n\tsha1rnds4 $1,%%xmm1,%%xmm2\n\t"
        "sha1rnds4 $2,%%xmm1,%%xmm3\n\tsha1rnds4 $3,%%xmm1,%%xmm4\n\t"
        "movdqu %%xmm0,%0\n\tmovdqu %%xmm2,%1\n\tmovdqu %%xmm3,%2\n\tmovdqu %%xmm4,%3"
        : "=m"(r0), "=m"(r1), "=m"(r2), "=m"(r3) : "m"(state), "m"(msg)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4");
    const xmm_t e0 = { .u32 = {0xf6f86db0,0x38b9e962,0xca055093,0x931064b4} };
    const xmm_t e1 = { .u32 = {0xafb0dbb3,0xf6e1abc2,0x154bf706,0x423373bd} };
    const xmm_t e2 = { .u32 = {0x841ebe81,0x8aa82f3d,0xd5e1a361,0x4fd450b8} };
    const xmm_t e3 = { .u32 = {0xc6931140,0x2a0a92f3,0xa0d57f8a,0x186e3058} };
    TEST_ASSERT(memcmp(&r0,&e0,16)==0 && memcmp(&r1,&e1,16)==0 && memcmp(&r2,&e2,16)==0 && memcmp(&r3,&e3,16)==0,
        "SHA1RNDS4 all four golden outputs");

    xmm_t a = { .u32 = {0x61626364,0x65666768,0x696A6B6C,0x6D6E6F70} };
    xmm_t b = { .u32 = {0x71727374,0x75767778,0x797A3031,0x32333435} }, r;
    __asm__ volatile("movdqu %1,%%xmm0\n\tmovdqu %2,%%xmm1\n\tsha1msg1 %%xmm1,%%xmm0\n\tmovdqu %%xmm0,%0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1");
    const xmm_t em1 = { .u32 = {0x18185355,0x5755535d,0x08080808,0x08080818} };
    TEST_ASSERT(memcmp(&r,&em1,16)==0, "SHA1MSG1 golden output");
    a = (xmm_t){ .u32 = {0x12345678,0x9ABCDEF0,0x0FEDCBA9,0x87654321} };
    b = (xmm_t){ .u32 = {0xAAAAAAAA,0xBBBBBBBB,0xCCCCCCCC,0xDDDDDDDD} };
    __asm__ volatile("movdqu %1,%%xmm0\n\tmovdqu %2,%%xmm1\n\tsha1msg2 %%xmm1,%%xmm0\n\tmovdqu %%xmm0,%0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1");
    const xmm_t em2 = { .u32 = {0x0ace9345,0x602ce8b4,0x68ace025,0x97531fda} };
    TEST_ASSERT(memcmp(&r,&em2,16)==0, "SHA1MSG2 golden output");
    a = (xmm_t){ .u32 = {0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476} };
    __asm__ volatile("movdqu %1,%%xmm0\n\tmovdqu %2,%%xmm1\n\tsha1nexte %%xmm1,%%xmm0\n\tmovdqu %%xmm0,%0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1");
    const xmm_t ene = { .u32 = {0xaaaaaaaa,0xbbbbbbbb,0xcccccccc,0x61ea72fa} };
    TEST_ASSERT(memcmp(&r,&ene,16)==0, "SHA1NEXTE golden output");

    xmm_t cdgh = { .u32 = {0x5be0cd19,0x1f83d9ab,0x9b05688c,0x510e527f} };
    xmm_t abef = { .u32 = {0xe9b5dba5,0xb5c0fbcf,0x71374491,0x6a09e667} };
    xmm_t wk = { .u32 = {0x5a827999,0x6ed9eba1,0,0} };
    __asm__ volatile("movdqu %1,%%xmm1\n\tmovdqu %2,%%xmm2\n\tmovdqu %3,%%xmm0\n\tsha256rnds2 %%xmm2,%%xmm1\n\tmovdqu %%xmm1,%0"
        : "=m"(r) : "m"(cdgh), "m"(abef), "m"(wk) : "xmm0", "xmm1", "xmm2");
    const xmm_t er = { .u32 = {0xa448e70d,0xc0b83fca,0x48737976,0x6c2896a9} };
    TEST_ASSERT(memcmp(&r,&er,16)==0, "SHA256RNDS2 golden output");
    a = (xmm_t){ .u32 = {0x61626380,0x12345678,0x9ABCDEF0,0xCAFEBABE} };
    b = (xmm_t){ .u32 = {0xDEADBEEF,0x11111111,0x22222222,0x33333333} };
    __asm__ volatile("movdqu %1,%%xmm0\n\tmovdqu %2,%%xmm1\n\tsha256msg1 %%xmm1,%%xmm0\n\tmovdqu %%xmm0,%0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1");
    const xmm_t es1 = { .u32 = {0x495f4a6e,0xd8131b44,0x6522778d,0x76d1d5c9} };
    TEST_ASSERT(memcmp(&r,&es1,16)==0, "SHA256MSG1 golden output");
    a = (xmm_t){ .u32 = {0x12345678,0x9ABCDEF0,0x0FEDCBA9,0x87654321} };
    b = (xmm_t){ .u32 = {0xAAAAAAAA,0xBBBBBBBB,0xCCCCCCCC,0xDDDDDDDD} };
    __asm__ volatile("movdqu %1,%%xmm0\n\tmovdqu %2,%%xmm1\n\tsha256msg2 %%xmm1,%%xmm0\n\tmovdqu %%xmm0,%0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1");
    const xmm_t es2 = { .u32 = {0x12012344,0xf01f0112,0xc5bbd6b1,0xe7fc64ed} };
    TEST_ASSERT(memcmp(&r,&es2,16)==0, "SHA256MSG2 golden output");
}

int main(void) {
    TEST_START("SHA1/SHA256 instructions");

    if (!check_sha()) {
        printf("SHA extensions not supported, skipping\n");
        return 0;
    }

    test_sha_golden_outputs();

    /* SHA1RNDS4: SHA1 round function with immediate for function select (0-3)
       SHA1RNDS4 xmm1, xmm2/m128, imm8
       Takes state (xmm1) and msg+const (xmm2), performs 4 rounds */
    {
        xmm_t state, msg, result;
        /* Initialize with known values */
        state.u32[0] = 0x67452301; state.u32[1] = 0xEFCDAB89;
        state.u32[2] = 0x98BADCFE; state.u32[3] = 0x10325476;
        msg.u32[0] = 0x11111111; msg.u32[1] = 0x22222222;
        msg.u32[2] = 0x33333333; msg.u32[3] = 0x44444444;
        __asm__ volatile(
            "movdqa %1, %%xmm0\n\t"
            "movdqa %2, %%xmm1\n\t"
            "sha1rnds4 $0, %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %0"
            : "=m"(result)
            : "m"(state), "m"(msg)
            : "xmm0", "xmm1");
        /* Verify output changed (deterministic but complex) */
        TEST_ASSERT(result.u32[0] != state.u32[0] || result.u32[1] != state.u32[1],
                    "sha1rnds4: state changed");
    }

    /* SHA1RNDS4 with function=1 (Parity) */
    {
        xmm_t state, msg, result;
        state.u32[0] = 0x67452301; state.u32[1] = 0xEFCDAB89;
        state.u32[2] = 0x98BADCFE; state.u32[3] = 0x10325476;
        msg.u32[0] = 0x55555555; msg.u32[1] = 0x66666666;
        msg.u32[2] = 0x77777777; msg.u32[3] = 0x88888888;
        __asm__ volatile(
            "movdqa %1, %%xmm0\n\t"
            "movdqa %2, %%xmm1\n\t"
            "sha1rnds4 $1, %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %0"
            : "=m"(result)
            : "m"(state), "m"(msg)
            : "xmm0", "xmm1");
        TEST_ASSERT(result.u32[0] != state.u32[0], "sha1rnds4 fn=1: state changed");
    }

    /* SHA1NEXTE: SHA1 next-E update - deterministic check (run twice, compare) */
    {
        xmm_t a, b, result1, result2;
        a.u32[0] = 0; a.u32[1] = 0; a.u32[2] = 0; a.u32[3] = 0x100;
        b.u32[0] = 0; b.u32[1] = 0; b.u32[2] = 0; b.u32[3] = 0x200;
        __asm__ volatile(
            "movdqa %2, %%xmm0\n\t"
            "movdqa %3, %%xmm1\n\t"
            "sha1nexte %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %0\n\t"
            "movdqa %2, %%xmm0\n\t"
            "sha1nexte %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %1"
            : "=m"(result1), "=m"(result2)
            : "m"(a), "m"(b)
            : "xmm0", "xmm1");
        TEST_ASSERT(result1.u32[3] == result2.u32[3] && result1.u32[3] != a.u32[3],
                    "sha1nexte: deterministic and changed r[3]=0x%x", result1.u32[3]);
    }

    /* SHA1MSG1: deterministic check */
    {
        xmm_t a, b, result1, result2;
        a.u32[0] = 0x11; a.u32[1] = 0x22; a.u32[2] = 0x33; a.u32[3] = 0x44;
        b.u32[0] = 0xAA; b.u32[1] = 0xBB; b.u32[2] = 0xCC; b.u32[3] = 0xDD;
        __asm__ volatile(
            "movdqa %2, %%xmm0\n\t"
            "movdqa %3, %%xmm1\n\t"
            "sha1msg1 %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %0\n\t"
            "movdqa %2, %%xmm0\n\t"
            "sha1msg1 %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %1"
            : "=m"(result1), "=m"(result2)
            : "m"(a), "m"(b)
            : "xmm0", "xmm1");
        int ok = 1;
        for (int i = 0; i < 4; i++) if (result1.u32[i] != result2.u32[i]) ok = 0;
        TEST_ASSERT(ok, "sha1msg1: deterministic output");
        TEST_ASSERT(result1.u32[0] != a.u32[0] || result1.u32[1] != a.u32[1],
                    "sha1msg1: output changed from input");
    }

    /* SHA1MSG2: final message schedule XOR and rotate */
    {
        xmm_t a, b, result;
        a.u32[0] = 0x1000; a.u32[1] = 0x2000; a.u32[2] = 0x3000; a.u32[3] = 0x4000;
        b.u32[0] = 0; b.u32[1] = 0; b.u32[2] = 0; b.u32[3] = 0xF0;
        __asm__ volatile(
            "movdqa %1, %%xmm0\n\t"
            "movdqa %2, %%xmm1\n\t"
            "sha1msg2 %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %0"
            : "=m"(result)
            : "m"(a), "m"(b)
            : "xmm0", "xmm1");
        /* sha1msg2 performs ROL1 of (a[i] XOR b[3]) for each element, cascading */
        TEST_ASSERT(result.u32[0] != a.u32[0], "sha1msg2: state changed");
    }

    /* SHA256RNDS2: SHA-256 round, uses implicit XMM0 for MSG */
    {
        xmm_t state1, state2, msg, result;
        state1.u32[0] = 0x6A09E667; state1.u32[1] = 0xBB67AE85;
        state1.u32[2] = 0x3C6EF372; state1.u32[3] = 0xA54FF53A;
        state2.u32[0] = 0x510E527F; state2.u32[1] = 0x9B05688C;
        state2.u32[2] = 0x1F83D9AB; state2.u32[3] = 0x5BE0CD19;
        msg.u32[0] = 0x428A2F98; msg.u32[1] = 0x71374491;
        msg.u32[2] = 0; msg.u32[3] = 0;
        __asm__ volatile(
            "movdqa %2, %%xmm0\n\t"   /* implicit msg */
            "movdqa %3, %%xmm1\n\t"   /* state1 */
            "movdqa %4, %%xmm2\n\t"   /* state2 */
            "sha256rnds2 %%xmm2, %%xmm1\n\t"
            "movdqa %%xmm1, %0\n\t"
            "movdqa %%xmm2, %1"
            : "=m"(result), "=m"(state2)
            : "m"(msg), "m"(state1), "m"(state2)
            : "xmm0", "xmm1", "xmm2");
        TEST_ASSERT(result.u32[0] != state1.u32[0], "sha256rnds2: state changed");
    }

    /* SHA256MSG1: sigma0 message schedule */
    {
        xmm_t a, b, result;
        a.u32[0] = 0x428A2F98; a.u32[1] = 0x71374491;
        a.u32[2] = 0xB5C0FBCF; a.u32[3] = 0xE9B5DBA5;
        b.u32[0] = 0x3956C25B; b.u32[1] = 0; b.u32[2] = 0; b.u32[3] = 0;
        __asm__ volatile(
            "movdqa %1, %%xmm0\n\t"
            "movdqa %2, %%xmm1\n\t"
            "sha256msg1 %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %0"
            : "=m"(result)
            : "m"(a), "m"(b)
            : "xmm0", "xmm1");
        TEST_ASSERT(result.u32[0] != a.u32[0], "sha256msg1: schedule updated");
    }

    /* SHA256MSG2: sigma1 message schedule */
    {
        xmm_t a, b, result;
        a.u32[0] = 0x11111111; a.u32[1] = 0x22222222;
        a.u32[2] = 0x33333333; a.u32[3] = 0x44444444;
        b.u32[0] = 0; b.u32[1] = 0;
        b.u32[2] = 0x55555555; b.u32[3] = 0x66666666;
        __asm__ volatile(
            "movdqa %1, %%xmm0\n\t"
            "movdqa %2, %%xmm1\n\t"
            "sha256msg2 %%xmm1, %%xmm0\n\t"
            "movdqa %%xmm0, %0"
            : "=m"(result)
            : "m"(a), "m"(b)
            : "xmm0", "xmm1");
        TEST_ASSERT(result.u32[0] != a.u32[0], "sha256msg2: schedule updated");
    }

    TEST_END();
}
