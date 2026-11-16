#ifndef X86_64_AVX512_BITALG_POPCNT_TEST_H
#define X86_64_AVX512_BITALG_POPCNT_TEST_H

#include "avx512_test.h"

#if TEST_BITS == 8
#define TEST_FIELD u8
#define TEST_LANES 64
#define TEST_KMOV "kmovq"
#define TEST_ENDPOINT_MASK UINT64_C(0x8000000000000001)
#define TEST_MIXED_MASK UINT64_C(0xa55aa55aa55aa55a)
#define TEST_SENTINEL UINT8_C(0xa5)
typedef uint8_t test_uint_t;
typedef uint64_t test_mask_t;
#elif TEST_BITS == 16
#define TEST_FIELD u16
#define TEST_LANES 32
#define TEST_KMOV "kmovd"
#define TEST_ENDPOINT_MASK UINT32_C(0x80000001)
#define TEST_MIXED_MASK UINT32_C(0xa55aa55a)
#define TEST_SENTINEL UINT16_C(0xa5a5)
typedef uint16_t test_uint_t;
typedef uint32_t test_mask_t;
#else
#error "TEST_BITS must be 8 or 16"
#endif

static test_uint_t expected_popcount(test_uint_t value) {
    return (test_uint_t)__builtin_popcountll((unsigned long long)value);
}

int main(void) {
    zmm_test_t src, initial, out;
    test_mask_t mask;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512BITALG()) return 0;
    for (int i = 0; i < TEST_LANES; i++) {
        src.TEST_FIELD[i] = (test_uint_t)(i * 977U + 11U);
        initial.TEST_FIELD[i] = TEST_SENTINEL;
    }
    src.TEST_FIELD[0] = 0;
    src.TEST_FIELD[1] = (test_uint_t)~(test_uint_t)0;

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\t"
                     TEST_INSN " %%zmm0, %%zmm1\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < TEST_LANES; i++)
        TEST_ASSERT(out.TEST_FIELD[i] == expected_popcount(src.TEST_FIELD[i]),
                    TEST_NAME " lane %d", i);

    mask = TEST_ENDPOINT_MASK;
    __asm__ volatile(TEST_KMOV " %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     TEST_INSN " %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < TEST_LANES; i++) {
        test_uint_t expected = (mask >> i) & 1U
            ? expected_popcount(src.TEST_FIELD[i]) : TEST_SENTINEL;
        TEST_ASSERT(out.TEST_FIELD[i] == expected,
                    TEST_NAME " merge mask lane %d", i);
    }

    mask = TEST_MIXED_MASK;
    __asm__ volatile(TEST_KMOV " %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     TEST_INSN " %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < TEST_LANES; i++) {
        test_uint_t expected = (mask >> i) & 1U
            ? expected_popcount(src.TEST_FIELD[i]) : 0;
        TEST_ASSERT(out.TEST_FIELD[i] == expected,
                    TEST_NAME " zero mask lane %d", i);
    }
    TEST_END();
}

#endif
