/* Additional AVX-512F/BW/DQ foundation forms supported by GAS 2.38. */
#include "avx512_test.h"

static uint64_t rol64(uint64_t value, unsigned count) {
    count &= 63U;
    return count ? (value << count) | (value >> (64U - count)) : value;
}

static uint64_t ror64(uint64_t value, unsigned count) {
    count &= 63U;
    return count ? (value >> count) | (value << (64U - count)) : value;
}

static void test_align_and_broadcast(void) {
    zmm_test_t a, b, out;
    for (int i = 0; i < 8; i++) {
        a.u64[i] = UINT64_C(0x1000000000000000) + (uint64_t)i;
        b.u64[i] = UINT64_C(0x2000000000000000) + (uint64_t)i;
    }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "valignq $2, %%zmm1, %%zmm0, %%zmm2\n\tvmovdqa64 %%zmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = i < 6 ? b.u64[i + 2] : a.u64[i - 6];
        TEST_ASSERT(out.u64[i] == expected, "VALIGNQ lane %d", i);
    }

    __asm__ volatile("vbroadcasti32x4 %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a) : "zmm0");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == a.u32[i & 3], "VBROADCASTI32X4 lane %d", i);

    __asm__ volatile("vbroadcastf64x2 %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(b) : "zmm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == b.u64[i & 1], "VBROADCASTF64X2 lane %d", i);
}

static void test_insert(void) {
    zmm_test_t base, out;
    xmm_t insert;
    for (int i = 0; i < 16; i++) base.u32[i] = UINT32_C(0x1000) + (uint32_t)i;
    for (int i = 0; i < 4; i++) insert.u32[i] = UINT32_C(0xfeed0000) + (uint32_t)i;
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\t"
                     "vinserti32x4 $2, %2, %%zmm0, %%zmm1\n\tvmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(base), "m"(insert) : "zmm0", "zmm1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = i >= 8 && i < 12 ? insert.u32[i - 8] : base.u32[i];
        TEST_ASSERT(out.u32[i] == expected, "VINSERTI32X4 lane %d", i);
    }
}

static void test_average_and_multiply(void) {
    zmm_test_t a, b, avg_b, avg_w, product, rounded;
    for (int i = 0; i < 64; i++) {
        a.u8[i] = (uint8_t)(i * 3U);
        b.u8[i] = (uint8_t)(255U - i * 2U);
    }
    __asm__ volatile("vmovdqa64 %4, %%zmm0\n\tvmovdqa64 %5, %%zmm1\n\t"
                     "vpavgb %%zmm1, %%zmm0, %%zmm2\n\t"
                     "vpavgw %%zmm1, %%zmm0, %%zmm3\n\t"
                     "vmovdqa64 %%zmm2, %0\n\tvmovdqa64 %%zmm3, %1\n\t"
                     "vpmuldq %%zmm1, %%zmm0, %%zmm4\n\tvmovdqa64 %%zmm4, %2"
                     : "=m"(avg_b), "=m"(avg_w), "=m"(product)
                     : "m"(rounded), "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4");
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(avg_b.u8[i] == ((unsigned)a.u8[i] + b.u8[i] + 1U) / 2U,
                    "VPAVGB lane %d", i);
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(avg_w.u16[i] == ((uint32_t)a.u16[i] + b.u16[i] + 1U) / 2U,
                    "VPAVGW lane %d", i);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(product.i64[i] == (int64_t)a.i32[i * 2] * b.i32[i * 2],
                    "VPMULDQ lane %d", i);

    for (int i = 0; i < 32; i++) {
        a.i16[i] = 16384;
        b.i16[i] = (int16_t)(i * 257);
    }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vpmulhrsw %%zmm1, %%zmm0, %%zmm2\n\tvmovdqa64 %%zmm2, %0"
                     : "=m"(rounded) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 32; i++) {
        int32_t expected = ((int32_t)a.i16[i] * b.i16[i] + 0x4000) >> 15;
        TEST_ASSERT(rounded.i16[i] == expected, "VPMULHRSW lane %d", i);
    }
}

static void test_rotate_and_shift(void) {
    zmm_test_t value, count, left, right, variable, shifted_left, shifted_right, arithmetic;
    const unsigned counts[8] = {0, 1, 7, 31, 63, 64, 65, 127};
    for (int i = 0; i < 8; i++) {
        value.u64[i] = UINT64_C(0x8123456789abcdef) ^ ((uint64_t)i << 56);
        count.u64[i] = counts[i];
    }
    __asm__ volatile("vmovdqa64 %6, %%zmm0\n\tvmovdqa64 %7, %%zmm1\n\t"
                     "vprolq $13, %%zmm0, %%zmm2\n\t"
                     "vprorq $7, %%zmm0, %%zmm3\n\t"
                     "vprolvq %%zmm1, %%zmm0, %%zmm4\n\t"
                     "vpsllvq %%zmm1, %%zmm0, %%zmm5\n\t"
                     "vpsrlvq %%zmm1, %%zmm0, %%zmm6\n\t"
                     "vpsravq %%zmm1, %%zmm0, %%zmm7\n\t"
                     "vmovdqa64 %%zmm2, %0\n\tvmovdqa64 %%zmm3, %1\n\t"
                     "vmovdqa64 %%zmm4, %2\n\tvmovdqa64 %%zmm5, %3\n\t"
                     "vmovdqa64 %%zmm6, %4\n\tvmovdqa64 %%zmm7, %5"
                     : "=m"(left), "=m"(right), "=m"(variable),
                       "=m"(shifted_left), "=m"(shifted_right), "=m"(arithmetic)
                     : "m"(value), "m"(count)
                     : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5", "zmm6", "zmm7");
    for (int i = 0; i < 8; i++) {
        unsigned c = counts[i];
        TEST_ASSERT(left.u64[i] == rol64(value.u64[i], 13), "VPROLQ lane %d", i);
        TEST_ASSERT(right.u64[i] == ror64(value.u64[i], 7), "VPRORQ lane %d", i);
        TEST_ASSERT(variable.u64[i] == rol64(value.u64[i], c), "VPROLVQ lane %d", i);
        TEST_ASSERT(shifted_left.u64[i] == (c < 64 ? value.u64[i] << c : 0),
                    "VPSLLVQ lane %d", i);
        TEST_ASSERT(shifted_right.u64[i] == (c < 64 ? value.u64[i] >> c : 0),
                    "VPSRLVQ lane %d", i);
        int64_t expected = c < 64 ? value.i64[i] >> c : (value.i64[i] < 0 ? -1 : 0);
        TEST_ASSERT(arithmetic.i64[i] == expected, "VPSRAVQ lane %d", i);
    }
}

int main(void) {
    TEST_START("Additional AVX-512 foundation forms");
    if (!CHECK_AVX512F()) return 0;
    test_align_and_broadcast();
    test_insert();
    test_average_and_multiply();
    test_rotate_and_shift();
    TEST_END();
}
