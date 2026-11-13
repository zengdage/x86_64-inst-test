/* Signed and unsigned AVX-512 byte, word, and qword compare predicates. */
#include "avx512_test.h"

static int predicate_signed(int64_t a, int64_t b, int predicate) {
    switch (predicate) {
    case 0: return a == b;
    case 1: return a < b;
    case 2: return a <= b;
    case 3: return 0;
    case 4: return a != b;
    case 5: return a >= b;
    case 6: return a > b;
    default: return 1;
    }
}

static int predicate_unsigned(uint64_t a, uint64_t b, int predicate) {
    switch (predicate) {
    case 0: return a == b;
    case 1: return a < b;
    case 2: return a <= b;
    case 3: return 0;
    case 4: return a != b;
    case 5: return a >= b;
    case 6: return a > b;
    default: return 1;
    }
}

#define RUN_COMPARE(INST, IMM, A, B, MASK) \
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t" \
                     #INST " $" #IMM ", %%zmm1, %%zmm0, %%k1\n\t" \
                     "kmovq %%k1, %0" \
                     : "=r"(MASK) : "m"(A), "m"(B) : "zmm0", "zmm1", "k1")

#define CHECK_BYTE_PRED(IMM) do { \
    uint64_t signed_mask, unsigned_mask, expected_signed = 0, expected_unsigned = 0; \
    RUN_COMPARE(vpcmpb, IMM, a, b, signed_mask); \
    RUN_COMPARE(vpcmpub, IMM, a, b, unsigned_mask); \
    for (int i = 0; i < 64; i++) { \
        expected_signed |= (uint64_t)predicate_signed(a.i8[i], b.i8[i], IMM) << i; \
        expected_unsigned |= (uint64_t)predicate_unsigned(a.u8[i], b.u8[i], IMM) << i; \
    } \
    TEST_ASSERT(signed_mask == expected_signed, "VPCMPB predicate %d", IMM); \
    TEST_ASSERT(unsigned_mask == expected_unsigned, "VPCMPUB predicate %d", IMM); \
} while (0)

static void test_bytes(void) {
    zmm_test_t a, b;
    for (int i = 0; i < 64; i++) {
        a.u8[i] = (uint8_t)(i * 17U + 0x70U);
        b.u8[i] = (uint8_t)((i % 5 == 0) ? a.u8[i] : 0x80U - i);
    }
    CHECK_BYTE_PRED(0); CHECK_BYTE_PRED(1); CHECK_BYTE_PRED(2); CHECK_BYTE_PRED(3);
    CHECK_BYTE_PRED(4); CHECK_BYTE_PRED(5); CHECK_BYTE_PRED(6); CHECK_BYTE_PRED(7);
}

#define CHECK_WORD_PRED(IMM) do { \
    uint64_t signed_mask, unsigned_mask, expected_signed = 0, expected_unsigned = 0; \
    RUN_COMPARE(vpcmpw, IMM, a, b, signed_mask); \
    RUN_COMPARE(vpcmpuw, IMM, a, b, unsigned_mask); \
    for (int i = 0; i < 32; i++) { \
        expected_signed |= (uint64_t)predicate_signed(a.i16[i], b.i16[i], IMM) << i; \
        expected_unsigned |= (uint64_t)predicate_unsigned(a.u16[i], b.u16[i], IMM) << i; \
    } \
    TEST_ASSERT((uint32_t)signed_mask == (uint32_t)expected_signed, "VPCMPW predicate %d", IMM); \
    TEST_ASSERT((uint32_t)unsigned_mask == (uint32_t)expected_unsigned, "VPCMPUW predicate %d", IMM); \
} while (0)

static void test_words(void) {
    zmm_test_t a, b;
    for (int i = 0; i < 32; i++) {
        a.u16[i] = (uint16_t)(i * 4099U + 0x7000U);
        b.u16[i] = (uint16_t)((i % 5 == 0) ? a.u16[i] : 0x8000U - i * 13U);
    }
    CHECK_WORD_PRED(0); CHECK_WORD_PRED(1); CHECK_WORD_PRED(2); CHECK_WORD_PRED(3);
    CHECK_WORD_PRED(4); CHECK_WORD_PRED(5); CHECK_WORD_PRED(6); CHECK_WORD_PRED(7);
}

#define CHECK_QWORD_PRED(IMM) do { \
    uint64_t signed_mask, unsigned_mask, expected_signed = 0, expected_unsigned = 0; \
    RUN_COMPARE(vpcmpq, IMM, a, b, signed_mask); \
    RUN_COMPARE(vpcmpuq, IMM, a, b, unsigned_mask); \
    for (int i = 0; i < 8; i++) { \
        expected_signed |= (uint64_t)predicate_signed(a.i64[i], b.i64[i], IMM) << i; \
        expected_unsigned |= (uint64_t)predicate_unsigned(a.u64[i], b.u64[i], IMM) << i; \
    } \
    TEST_ASSERT((uint8_t)signed_mask == (uint8_t)expected_signed, "VPCMPQ predicate %d", IMM); \
    TEST_ASSERT((uint8_t)unsigned_mask == (uint8_t)expected_unsigned, "VPCMPUQ predicate %d", IMM); \
} while (0)

static void test_qwords(void) {
    zmm_test_t a, b;
    for (int i = 0; i < 8; i++) {
        a.u64[i] = UINT64_C(0x7000000000000000) + (uint64_t)i * UINT64_C(0x1111111111111111);
        b.u64[i] = i % 3 == 0 ? a.u64[i] : UINT64_C(0x8000000000000000) - (uint64_t)i;
    }
    CHECK_QWORD_PRED(0); CHECK_QWORD_PRED(1); CHECK_QWORD_PRED(2); CHECK_QWORD_PRED(3);
    CHECK_QWORD_PRED(4); CHECK_QWORD_PRED(5); CHECK_QWORD_PRED(6); CHECK_QWORD_PRED(7);
}

#undef CHECK_QWORD_PRED
#undef CHECK_WORD_PRED
#undef CHECK_BYTE_PRED
#undef RUN_COMPARE

int main(void) {
    TEST_START("AVX-512 byte, word, and qword compare predicates");
    if (!CHECK_AVX512F()) return 0;
    test_bytes();
    test_words();
    test_qwords();
    TEST_END();
}
