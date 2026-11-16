/* Additional AVX-512F/BW packed integer operations with per-lane oracles. */
#include "../include/avx512_test.h"
#include <limits.h>

static int16_t saturate_i16(int32_t value) {
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static int16_t arithmetic_shift_right_i16(int16_t value, unsigned count) {
    uint16_t bits = (uint16_t)value;

    if (count >= 16) return value < 0 ? -1 : 0;
    if (count == 0) return value;
    bits = (uint16_t)(bits >> count);
    if (value < 0)
        bits |= (uint16_t)(UINT16_MAX << (16U - count));
    return (int16_t)bits;
}

static void test_maddubs(void) {
    zmm_test_t unsigned_bytes, signed_bytes, out, memory_out;
    for (int i = 0; i < 64; i++) {
        unsigned_bytes.u8[i] = (uint8_t)(i * 29U + 3U);
        signed_bytes.i8[i] = (int8_t)(i * 37U + 11U);
    }

    __asm__ volatile(
        "vmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
        "vpmaddubsw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqa64 %%zmm2, %0\n\t"
        "vpmaddubsw %3, %%zmm0, %%zmm3\n\t"
        "vmovdqa64 %%zmm3, %1"
        : "=m"(out), "=m"(memory_out)
        : "m"(unsigned_bytes), "m"(signed_bytes)
        : "zmm0", "zmm1", "zmm2", "zmm3");
    for (int i = 0; i < 32; i++) {
        int32_t expected =
            (int32_t)unsigned_bytes.u8[i * 2] * signed_bytes.i8[i * 2] +
            (int32_t)unsigned_bytes.u8[i * 2 + 1] * signed_bytes.i8[i * 2 + 1];
        expected = saturate_i16(expected);
        TEST_ASSERT(out.i16[i] == expected, "VPMADDUBSW register lane %d", i);
        TEST_ASSERT(memory_out.i16[i] == expected, "VPMADDUBSW memory lane %d", i);
    }

    for (int i = 0; i < 32; i++) {
        unsigned_bytes.u8[i * 2] = UINT8_MAX;
        unsigned_bytes.u8[i * 2 + 1] = UINT8_MAX;
        signed_bytes.i8[i * 2] = (i & 1) ? INT8_MIN : INT8_MAX;
        signed_bytes.i8[i * 2 + 1] = signed_bytes.i8[i * 2];
    }
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvpmaddubsw %2, %%zmm0, %%zmm1\n\t"
        "vmovdqa64 %%zmm1, %0"
        : "=m"(out) : "m"(unsigned_bytes), "m"(signed_bytes)
        : "zmm0", "zmm1");
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(out.i16[i] == ((i & 1) ? INT16_MIN : INT16_MAX),
                    "VPMADDUBSW saturation lane %d", i);

    const uint32_t mask = UINT32_C(0x80018021);
    zmm_test_t initial, merged, zeroed;
    for (int i = 0; i < 32; i++) initial.u16[i] = UINT16_C(0x5a5a);
    __asm__ volatile(
        "kmovd %4, %%k1\n\tvmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
        "vmovdqa64 %5, %%zmm2\n\t"
        "vpmaddubsw %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vpmaddubsw %%zmm1, %%zmm0, %%zmm3%{%%k1%}%{z%}\n\t"
        "vmovdqa64 %%zmm2, %0\n\tvmovdqa64 %%zmm3, %1"
        : "=m"(merged), "=m"(zeroed)
        : "m"(unsigned_bytes), "m"(signed_bytes), "r"(mask), "m"(initial)
        : "zmm0", "zmm1", "zmm2", "zmm3", "k1");
    for (int i = 0; i < 32; i++) {
        int16_t active = (i & 1) ? INT16_MIN : INT16_MAX;
        TEST_ASSERT(merged.u16[i] == ((mask >> i) & 1U ? (uint16_t)active : UINT16_C(0x5a5a)),
                    "VPMADDUBSW merge mask lane %d", i);
        TEST_ASSERT(zeroed.i16[i] == ((mask >> i) & 1U ? active : 0),
                    "VPMADDUBSW zero mask lane %d", i);
    }
}

static void test_mulld(void) {
    static const uint32_t left_values[16] = {
        0, 1, UINT32_MAX, UINT32_C(0x80000000),
        UINT32_C(0x7fffffff), UINT32_C(0x10001), UINT32_C(0xdeadbeef), 3,
        5, 7, 11, 13, 17, 19, 23, 29
    };
    static const uint32_t right_values[16] = {
        UINT32_MAX, 2, UINT32_MAX, 2,
        UINT32_C(0x7fffffff), UINT32_C(0xffff), UINT32_C(0x12345678), UINT32_MAX,
        9, 11, 13, 15, 17, 19, 23, UINT32_C(0x80000000)
    };
    zmm_test_t left, right, out, memory_out;
    memcpy(left.u32, left_values, sizeof(left_values));
    memcpy(right.u32, right_values, sizeof(right_values));

    __asm__ volatile(
        "vmovdqa32 %2, %%zmm0\n\tvmovdqa32 %3, %%zmm1\n\t"
        "vpmulld %%zmm1, %%zmm0, %%zmm2\n\tvmovdqa32 %%zmm2, %0\n\t"
        "vpmulld %3, %%zmm0, %%zmm0\n\tvmovdqa32 %%zmm0, %1"
        : "=m"(out), "=m"(memory_out) : "m"(left), "m"(right)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (uint32_t)((uint64_t)left.u32[i] * right.u32[i]);
        TEST_ASSERT(out.u32[i] == expected, "VPMULLD register lane %d", i);
        TEST_ASSERT(memory_out.u32[i] == expected,
                    "VPMULLD memory and overlapping destination lane %d", i);
    }

    const uint32_t mask = UINT32_C(0x8001);
    __asm__ volatile(
        "kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t"
        "vpmulld %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\tvmovdqa32 %%zmm2, %0"
        : "=m"(out) : "m"(left), "m"(right), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t product = (uint32_t)((uint64_t)left.u32[i] * right.u32[i]);
        TEST_ASSERT(out.u32[i] == (i == 0 || i == 15 ? product : 0),
                    "VPMULLD endpoint zero mask lane %d", i);
    }
}

static void test_variable_word_shifts(void) {
    static const uint16_t counts[32] = {
        0, 1, 7, 14, 15, 16, 17, 31,
        UINT16_MAX, 2, 3, 4, 5, 6, 8, 9,
        10, 11, 12, 13, 15, 16, 32, 255,
        0, 1, 14, 15, 16, 17, UINT16_C(0x8000), UINT16_MAX
    };
    zmm_test_t value, count, left, logical, arithmetic;
    for (int i = 0; i < 32; i++) {
        value.u16[i] = (uint16_t)(UINT16_C(0x8001) ^ (uint16_t)(i * 0x1111U));
        count.u16[i] = counts[i];
    }

    __asm__ volatile(
        "vmovdqa64 %3, %%zmm0\n\tvmovdqa64 %4, %%zmm1\n\t"
        "vpsllvw %%zmm1, %%zmm0, %%zmm2\n\t"
        "vpsrlvw %%zmm1, %%zmm0, %%zmm3\n\t"
        "vpsravw %%zmm1, %%zmm0, %%zmm4\n\t"
        "vmovdqa64 %%zmm2, %0\n\tvmovdqa64 %%zmm3, %1\n\t"
        "vmovdqa64 %%zmm4, %2"
        : "=m"(left), "=m"(logical), "=m"(arithmetic)
        : "m"(value), "m"(count)
        : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4");
    for (int i = 0; i < 32; i++) {
        unsigned c = count.u16[i];
        uint16_t expected_left = c < 16 ? (uint16_t)((uint32_t)value.u16[i] << c) : 0;
        uint16_t expected_right = c < 16 ? (uint16_t)(value.u16[i] >> c) : 0;
        int16_t expected_arithmetic = arithmetic_shift_right_i16(value.i16[i], c);
        TEST_ASSERT(left.u16[i] == expected_left, "VPSLLVW lane %d count %u", i, c);
        TEST_ASSERT(logical.u16[i] == expected_right, "VPSRLVW lane %d count %u", i, c);
        TEST_ASSERT(arithmetic.i16[i] == expected_arithmetic,
                    "VPSRAVW lane %d count %u", i, c);
    }

    const uint32_t mask = UINT32_C(0x80000001);
    __asm__ volatile(
        "kmovd %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
        "vpsllvw %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\tvmovdqa64 %%zmm2, %0"
        : "=m"(left) : "m"(value), "m"(count), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 32; i++) {
        unsigned c = count.u16[i];
        uint16_t shifted = c < 16 ? (uint16_t)((uint32_t)value.u16[i] << c) : 0;
        TEST_ASSERT(left.u16[i] == (i == 0 || i == 31 ? shifted : 0),
                    "VPSLLVW endpoint zero mask lane %d", i);
    }
}

int main(void) {
    TEST_START("Additional AVX-512 packed integer lane operations");
    if (!CHECK_AVX512F() || !CHECK_AVX512BW()) return 0;
    test_maddubs();
    test_mulld();
    test_variable_word_shifts();
    TEST_END();
}
