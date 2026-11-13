/* AVX-512BITALG byte/word population count and bit shuffle to mask. */
#include "avx512_test.h"

static unsigned popcount8(uint8_t value) {
    unsigned count = 0;
    for (; value; value &= (uint8_t)(value - 1)) count++;
    return count;
}

static unsigned popcount16(uint16_t value) {
    unsigned count = 0;
    for (; value; value &= (uint16_t)(value - 1)) count++;
    return count;
}

static void test_popcount(void) {
    zmm_test_t src, bytes, words;
    for (int i = 0; i < 64; i++) src.u8[i] = (uint8_t)(i * 37U + 11U);
    src.u8[0] = 0;
    src.u8[1] = UINT8_MAX;
    src.u16[1] = UINT16_MAX;
    __asm__ volatile("vmovdqa64 %2, %%zmm0\n\tvpopcntb %%zmm0, %%zmm1\n\t"
                     "vpopcntw %%zmm0, %%zmm2\n\tvmovdqa64 %%zmm1, %0\n\t"
                     "vmovdqa64 %%zmm2, %1"
                     : "=m"(bytes), "=m"(words) : "m"(src)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(bytes.u8[i] == popcount8(src.u8[i]), "VPOPCNTB lane %d", i);
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(words.u16[i] == popcount16(src.u16[i]), "VPOPCNTW lane %d", i);

    uint64_t mask = UINT64_C(0x8000000000000001);
    __asm__ volatile("kmovq %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vpopcntb %%zmm0, %%zmm1%{%%k1%}%{z%}\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(bytes) : "m"(src), "r"(mask) : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 64; i++) {
        uint8_t expected = (i == 0 || i == 63) ? (uint8_t)popcount8(src.u8[i]) : 0;
        TEST_ASSERT(bytes.u8[i] == expected, "VPOPCNTB zero mask lane %d", i);
    }
}

static void test_bitshuffle(void) {
    zmm_test_t data, control;
    for (int q = 0; q < 8; q++) {
        data.u64[q] = UINT64_C(0x0123456789abcdef) ^
                      (UINT64_C(0x1111111111111111) * (uint64_t)q);
        for (int j = 0; j < 8; j++)
            control.u8[q * 8 + j] = (uint8_t)((j * 9 + q) & 63);
    }
    uint64_t result;
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vpshufbitqmb %%zmm1, %%zmm0, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(result) : "m"(data), "m"(control)
                     : "zmm0", "zmm1", "k1");
    uint64_t expected = 0;
    for (int i = 0; i < 64; i++)
        expected |= ((data.u64[i / 8] >> (control.u8[i] & 63U)) & 1U) << i;
    TEST_ASSERT(result == expected, "VPSHUFBITQMB result 0x%016" PRIx64, result);

    uint64_t writemask = UINT64_C(0x8000000000000001);
    __asm__ volatile("kmovq %3, %%k2\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpshufbitqmb %%zmm1, %%zmm0, %%k1%{%%k2%}\n\tkmovq %%k1, %0"
                     : "=r"(result) : "m"(data), "m"(control), "r"(writemask)
                     : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT(result == (expected & writemask), "VPSHUFBITQMB writemask");
}

int main(void) {
    TEST_START("AVX-512BITALG VPOPCNTB/W and VPSHUFBITQMB");
    if (!CHECK_AVX512BITALG()) return 0;
    test_popcount();
    test_bitshuffle();
    TEST_END();
}
