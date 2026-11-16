/*
 * TARGETS: VPMOVSXBW VPMOVSXBD VPMOVSXBQ VPMOVSXDQ VPMOVSXWD VPMOVSXWQ
 * VPMOVZXBW VPMOVZXBD VPMOVZXBQ VPMOVZXDQ VPMOVZXWD VPMOVZXWQ
 */
#include "../common.h"

int main(void) {
    const xmm_t src = { .u8 = {
        0x80, 0xff, 0x00, 0x01, 0x7f, 0x55, 0xaa, 0xfe,
        0x34, 0x12, 0xcc, 0xed, 0xef, 0xcd, 0xab, 0x89
    } };
    ymm_t out;

    TEST_START("AVX2 sign/zero-extension move gaps");
    __asm__ volatile("vpmovsxbw %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.i16[i] == (int16_t)src.i8[i], "VPMOVSXBW ymm lane %d", i);

    __asm__ volatile("vpmovsxbd %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.i32[i] == (int32_t)src.i8[i], "VPMOVSXBD lane %d", i);

    __asm__ volatile("vpmovsxbq %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.i64[i] == (int64_t)src.i8[i], "VPMOVSXBQ lane %d", i);

    __asm__ volatile("vpmovsxdq %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.i64[i] == (int64_t)src.i32[i], "VPMOVSXDQ lane %d", i);

    __asm__ volatile("vpmovsxwd %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.i32[i] == (int32_t)src.i16[i], "VPMOVSXWD lane %d", i);

    __asm__ volatile("vpmovsxwq %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.i64[i] == (int64_t)src.i16[i], "VPMOVSXWQ lane %d", i);

    __asm__ volatile("vpmovzxbd %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == (uint32_t)src.u8[i], "VPMOVZXBD lane %d", i);

    __asm__ volatile("vpmovzxbw %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u16[i] == (uint16_t)src.u8[i], "VPMOVZXBW ymm lane %d", i);

    __asm__ volatile("vpmovzxbq %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == (uint64_t)src.u8[i], "VPMOVZXBQ lane %d", i);

    __asm__ volatile("vpmovzxdq %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == (uint64_t)src.u32[i], "VPMOVZXDQ lane %d", i);

    __asm__ volatile("vpmovzxwd %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == (uint32_t)src.u16[i], "VPMOVZXWD lane %d", i);

    __asm__ volatile("vpmovzxwq %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == (uint64_t)src.u16[i], "VPMOVZXWQ lane %d", i);

    ymm_t initial;
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmovsxbq %2, %%xmm0\n\t"
                     "vmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(initial), "m"(src) : "ymm0");
    for (int i = 0; i < 2; i++)
        TEST_ASSERT(out.i64[i] == (int64_t)src.i8[i], "VPMOVSXBQ xmm lane %d", i);
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0,
                "VPMOVSXBQ VEX.128 clears upper YMM");

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\tvmovdqu %2, %%xmm0\n\t"
                     "vpmovsxbw %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(src) : "xmm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.i16[i] == (int16_t)src.i8[i], "VPMOVSXBW xmm lane %d", i);
    for (int i = 8; i < 16; i++)
        TEST_ASSERT(out.u16[i] == 0, "VPMOVSXBW VEX.128 upper lane %d", i);

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\tvmovdqu %2, %%xmm0\n\t"
                     "vpmovzxbw %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(src) : "xmm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u16[i] == (uint16_t)src.u8[i], "VPMOVZXBW xmm lane %d", i);
    for (int i = 8; i < 16; i++)
        TEST_ASSERT(out.u16[i] == 0, "VPMOVZXBW VEX.128 upper lane %d", i);
    __asm__ volatile("vzeroupper");
    TEST_END();
}
