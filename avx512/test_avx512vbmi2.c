/* AVX-512VBMI2 byte/word compress-expand and double-vector shifts. */
#include "avx512_test.h"

static uint32_t shld32(uint32_t a, uint32_t b, unsigned count) {
    count &= 31U;
    return count ? (a << count) | (b >> (32U - count)) : a;
}

static uint32_t shrd32(uint32_t a, uint32_t b, unsigned count) {
    count &= 31U;
    return count ? (a >> count) | (b << (32U - count)) : a;
}

static void test_compress_expand(void) {
    zmm_test_t src, compressed, expanded;
    for (int i = 0; i < 64; i++) src.u8[i] = (uint8_t)(i + 1);
    uint64_t mask = UINT64_C(0x8000000100008429);
    __asm__ volatile("kmovq %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vpcompressb %%zmm0, %%zmm1%{%%k1%}%{z%}\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(compressed) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    int out = 0;
    for (int i = 0; i < 64; i++)
        if ((mask >> i) & 1U) TEST_ASSERT(compressed.u8[out++] == src.u8[i],
                                          "VPCOMPRESSB packed byte %d", out - 1);
    for (int i = out; i < 64; i++)
        TEST_ASSERT(compressed.u8[i] == 0, "VPCOMPRESSB zero tail %d", i);

    uint32_t word_mask = UINT32_C(0x80018429);
    for (int i = 0; i < 32; i++) src.u16[i] = (uint16_t)(0x1000 + i);
    __asm__ volatile("kmovd %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vpexpandw %%zmm0, %%zmm1%{%%k1%}%{z%}\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(expanded) : "m"(src), "r"(word_mask)
                     : "zmm0", "zmm1", "k1");
    int input = 0;
    for (int i = 0; i < 32; i++) {
        uint16_t expected = ((word_mask >> i) & 1U) ? src.u16[input++] : 0;
        TEST_ASSERT(expanded.u16[i] == expected, "VPEXPANDW lane %d", i);
    }
}

static void test_double_shifts(void) {
    zmm_test_t a, b, count, left, right, immediate;
    const uint32_t counts[16] = {0, 1, 4, 15, 31, 32, 33, 63, 2, 3, 5, 7, 8, 16, 24, 255};
    for (int i = 0; i < 16; i++) {
        a.u32[i] = UINT32_C(0x80000001) ^ (uint32_t)i * UINT32_C(0x01010101);
        b.u32[i] = UINT32_C(0x12345678) + (uint32_t)i * UINT32_C(0x11111111);
        count.u32[i] = counts[i];
    }
    __asm__ volatile("vmovdqa32 %3, %%zmm0\n\tvmovdqa32 %4, %%zmm1\n\t"
                     "vmovdqa32 %5, %%zmm2\n\t"
                     "vpshldvd %%zmm2, %%zmm1, %%zmm0\n\tvmovdqa32 %%zmm0, %0\n\t"
                     "vmovdqa32 %3, %%zmm0\n\tvpshrdvd %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %1\n\t"
                     "vmovdqa32 %3, %%zmm0\n\t"
                     "vpshldd $4, %%zmm1, %%zmm0, %%zmm3\n\tvmovdqa32 %%zmm3, %2"
                     : "=m"(left), "=m"(right), "=m"(immediate)
                     : "m"(a), "m"(b), "m"(count)
                     : "zmm0", "zmm1", "zmm2", "zmm3");
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(left.u32[i] == shld32(a.u32[i], b.u32[i], count.u32[i]),
                    "VPSHLDVD lane %d", i);
        TEST_ASSERT(right.u32[i] == shrd32(a.u32[i], b.u32[i], count.u32[i]),
                    "VPSHRDVD lane %d", i);
        TEST_ASSERT(immediate.u32[i] == shld32(a.u32[i], b.u32[i], 4),
                    "VPSHLDD immediate lane %d", i);
    }
}

int main(void) {
    TEST_START("AVX-512VBMI2 compress/expand and double shifts");
    if (!CHECK_AVX512VBMI2()) return 0;
    test_compress_expand();
    test_double_shifts();
    TEST_END();
}
