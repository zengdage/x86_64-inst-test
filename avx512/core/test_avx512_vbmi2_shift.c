/* AVX-512VBMI2 funnel shifts: VPSHLDW/Q, VPSHRDW/D/Q and the variable count
 * forms VPSHLDVQ/VPSHRDVQ.
 */
#include "../include/avx512_test.h"

/* The immediate forms funnel two sources into one.  With the AT&T operand
 * order "vpshldw $imm, %low, %high, %dst" the high source is shifted left and
 * the low source supplies the bits shifted in from the right.
 */
#define CHECK_IMM16(IMM) do { \
    for (int lane = 0; lane < 32; lane++) { \
        uint32_t high = high_src.u16[lane]; \
        uint32_t low = low_src.u16[lane]; \
        uint32_t expected = ((high << (IMM)) | (low >> (16 - (IMM)))) & 0xffffU; \
        TEST_ASSERT(dst.u16[lane] == expected, "shift left lane %d", lane); \
    } \
} while (0)

#define CHECK_IMMR16(IMM) do { \
    for (int lane = 0; lane < 32; lane++) { \
        uint32_t high = high_src.u16[lane]; \
        uint32_t low = low_src.u16[lane]; \
        uint32_t expected = ((high >> (IMM)) | (low << (16 - (IMM)))) & 0xffffU; \
        TEST_ASSERT(dst.u16[lane] == expected, "shift right lane %d", lane); \
    } \
} while (0)

#define CHECK_IMM32(IMM) do { \
    for (int lane = 0; lane < 16; lane++) { \
        uint32_t high = high_src.u32[lane]; \
        uint32_t low = low_src.u32[lane]; \
        uint32_t expected = ((high << (IMM)) | (low >> (32 - (IMM)))); \
        TEST_ASSERT(dst.u32[lane] == expected, "shift left lane %d", lane); \
    } \
} while (0)

#define CHECK_IMMR32(IMM) do { \
    for (int lane = 0; lane < 16; lane++) { \
        uint32_t high = high_src.u32[lane]; \
        uint32_t low = low_src.u32[lane]; \
        uint32_t expected = ((high >> (IMM)) | (low << (32 - (IMM)))); \
        TEST_ASSERT(dst.u32[lane] == expected, "shift right lane %d", lane); \
    } \
} while (0)

#define CHECK_IMM64(IMM) do { \
    for (int lane = 0; lane < 8; lane++) { \
        uint64_t high = high_src.u64[lane]; \
        uint64_t low = low_src.u64[lane]; \
        uint64_t expected = (high << (IMM)) | (low >> (64 - (IMM))); \
        TEST_ASSERT(dst.u64[lane] == expected, "shift left lane %d", lane); \
    } \
} while (0)

#define CHECK_IMMR64(IMM) do { \
    for (int lane = 0; lane < 8; lane++) { \
        uint64_t high = high_src.u64[lane]; \
        uint64_t low = low_src.u64[lane]; \
        uint64_t expected = (high >> (IMM)) | (low << (64 - (IMM))); \
        TEST_ASSERT(dst.u64[lane] == expected, "shift right lane %d", lane); \
    } \
} while (0)

static void test_immediate_shifts(void) {
    zmm_test_t low_src, high_src, dst;

    for (int i = 0; i < 32; i++) {
        low_src.u16[i] = (uint16_t)(0x1000U + (uint32_t)i);
        high_src.u16[i] = (uint16_t)(0x2000U + (uint32_t)i);
    }
    __asm__ volatile("vmovdqu16 %1, %%zmm0\n\tvmovdqu16 %2, %%zmm1\n\t"
                     "vpshldw $0x4, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu16 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMM16(4);

    __asm__ volatile("vmovdqu16 %1, %%zmm0\n\tvmovdqu16 %2, %%zmm1\n\t"
                     "vpshrdw $0x4, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu16 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMMR16(4);

    __asm__ volatile("vmovdqu16 %1, %%zmm0\n\tvmovdqu16 %2, %%zmm1\n\t"
                     "vpshldw $0xf, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu16 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMM16(15);

    __asm__ volatile("vmovdqu16 %1, %%zmm0\n\tvmovdqu16 %2, %%zmm1\n\t"
                     "vpshrdw $0xf, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu16 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMMR16(15);

    for (int i = 0; i < 8; i++) {
        low_src.u64[i] = UINT64_C(0x0123456789abcdef) * (uint64_t)(i + 1);
        high_src.u64[i] = UINT64_C(0xfedcba9876543210) ^ (UINT64_C(0x1111111111111111) * (uint64_t)i);
    }
    __asm__ volatile("vmovdqu64 %1, %%zmm0\n\tvmovdqu64 %2, %%zmm1\n\t"
                     "vpshldq $0x14, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu64 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMM64(20);

    __asm__ volatile("vmovdqu64 %1, %%zmm0\n\tvmovdqu64 %2, %%zmm1\n\t"
                     "vpshrdq $0x14, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu64 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMMR64(20);

    __asm__ volatile("vmovdqu64 %1, %%zmm0\n\tvmovdqu64 %2, %%zmm1\n\t"
                     "vpshldq $0x1, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu64 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMM64(1);

    __asm__ volatile("vmovdqu64 %1, %%zmm0\n\tvmovdqu64 %2, %%zmm1\n\t"
                     "vpshrdq $0x1, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu64 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMMR64(1);

    for (int i = 0; i < 16; i++) {
        low_src.u32[i] = UINT32_C(0x89abcdef) * (uint32_t)(i + 1);
        high_src.u32[i] = UINT32_C(0x76543210) ^ (UINT32_C(0x11111111) * (uint32_t)i);
    }
    __asm__ volatile("vmovdqu32 %1, %%zmm0\n\tvmovdqu32 %2, %%zmm1\n\t"
                     "vpshrdd $0xc, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu32 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMMR32(12);

    __asm__ volatile("vmovdqu32 %1, %%zmm0\n\tvmovdqu32 %2, %%zmm1\n\t"
                     "vpshrdd $0x1f, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu32 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMMR32(31);

    __asm__ volatile("vmovdqu32 %1, %%zmm0\n\tvmovdqu32 %2, %%zmm1\n\t"
                     "vpshrdd $0x1, %%zmm0, %%zmm1, %%zmm2\n\tvmovdqu32 %%zmm2, %0"
                     : "=m"(dst) : "m"(low_src), "m"(high_src)
                     : "zmm0", "zmm1", "zmm2");
    CHECK_IMMR32(1);
}

static void test_variable_qword_shifts(void) {
    zmm_test_t low_src, high_src, count, left, right, merged, zeroed;
    const unsigned counts[8] = {0, 1, 7, 31, 63, 64, 65, 127};

    for (int i = 0; i < 8; i++) {
        low_src.u64[i] = UINT64_C(0x0123456789abcdef) * (uint64_t)(i + 1);
        high_src.u64[i] = UINT64_C(0xfedcba9876543210) ^
                          (UINT64_C(0x1111111111111111) * (uint64_t)i);
        count.u64[i] = counts[i];
    }

    /* The variable forms take the count vector first in AT&T syntax. */
    __asm__ volatile(
        "vmovdqu64 %3, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vmovdqu64 %4, %%zmm2\n\t"
        "vpshldvq %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovdqu64 %3, %%zmm3\n\t"
        "vpshrdvq %%zmm2, %%zmm1, %%zmm3\n\t"
        "vmovdqu64 %%zmm0, %0\n\t"
        "vmovdqu64 %%zmm3, %1"
        : "=m"(left), "=m"(right)
        : "m"(low_src), "m"(high_src), "m"(count)
        : "zmm0", "zmm1", "zmm2", "zmm3");

    for (int i = 0; i < 8; i++) {
        unsigned c = counts[i] & 63U;
        uint64_t expected_left = c == 0 ? high_src.u64[i] :
            (high_src.u64[i] << c) | (low_src.u64[i] >> (64U - c));
        uint64_t expected_right = c == 0 ? high_src.u64[i] :
            (high_src.u64[i] >> c) | (low_src.u64[i] << (64U - c));
        TEST_ASSERT(left.u64[i] == expected_left,
                    "VPSHLDVQ lane %d count %u", i, counts[i]);
        TEST_ASSERT(right.u64[i] == expected_right,
                    "VPSHRDVQ lane %d count %u", i, counts[i]);
    }

    const uint32_t mask = UINT32_C(0x81);
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovdqu64 %4, %%zmm0\n\t"
        "vmovdqu64 %2, %%zmm1\n\t"
        "vmovdqu64 %5, %%zmm2\n\t"
        "vpshldvq %%zmm2, %%zmm1, %%zmm0%{%%k1%}\n\t"
        "vmovdqu64 %%zmm0, %0\n\t"
        "vmovdqu64 %4, %%zmm0\n\t"
        "vpshrdvq %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
        "vmovdqu64 %%zmm0, %1"
        : "=m"(merged), "=m"(zeroed)
        : "m"(low_src), "r"(mask), "m"(high_src), "m"(count)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) {
        unsigned c = counts[i] & 63U;
        uint64_t shifted = c == 0 ? high_src.u64[i] :
            (high_src.u64[i] << c) | (low_src.u64[i] >> (64U - c));
        uint64_t shifted_right = c == 0 ? high_src.u64[i] :
            (high_src.u64[i] >> c) | (low_src.u64[i] << (64U - c));
        TEST_ASSERT(merged.u64[i] == ((mask >> i) & 1U ? shifted : high_src.u64[i]),
                    "VPSHLDVQ merge mask lane %d", i);
        TEST_ASSERT(zeroed.u64[i] == ((mask >> i) & 1U ? shifted_right : 0),
                    "VPSHRDVQ zero mask lane %d", i);
    }
}

int main(void) {
    TEST_START("AVX-512VBMI2 funnel shifts");
    if (!CHECK_AVX512VBMI2()) return 0;
    test_immediate_shifts();
    test_variable_qword_shifts();
    TEST_END();
}
