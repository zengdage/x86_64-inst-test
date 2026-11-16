#ifndef X86_64_AVX512_CMP_PACKED_TEST_H
#define X86_64_AVX512_CMP_PACKED_TEST_H

#include "avx512_test.h"
#include <math.h>

#if TEST_BITS == 32
#define TEST_FIELD f32
#define TEST_LANES 16
#define TEST_KMOV_OUT "kmovw"
#define TEST_ALL_MASK UINT32_C(0xffff)
typedef float test_float_t;
#elif TEST_BITS == 64
#define TEST_FIELD f64
#define TEST_LANES 8
#define TEST_KMOV_OUT "kmovb"
#define TEST_ALL_MASK UINT32_C(0xff)
typedef double test_float_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

static uint32_t expected_mask(const zmm_test_t *a, const zmm_test_t *b,
                              int predicate) {
    uint32_t result = 0;
    for (int i = 0; i < TEST_LANES; i++) {
        test_float_t av = a->TEST_FIELD[i], bv = b->TEST_FIELD[i];
        int unordered = isnan(av) || isnan(bv);
        int value = 0;
        switch (predicate) {
            case 0: value = !unordered && av == bv; break;
            case 1: value = !unordered && av < bv; break;
            case 3: value = unordered; break;
            case 4: value = unordered || av != bv; break;
            case 7: value = !unordered; break;
        }
        if (value) result |= UINT32_C(1) << i;
    }
    return result;
}

#define RUN_COMPARE(IMM) do {                                                  \
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"      \
                     TEST_INSN " $" #IMM ", %%zmm1, %%zmm0, %%k1\n\t"      \
                     TEST_KMOV_OUT " %%k1, %0"                               \
                     : "=r"(result) : "m"(a), "m"(b)                       \
                     : "zmm0", "zmm1", "k1");                              \
    TEST_ASSERT((result & TEST_ALL_MASK) == expected_mask(&a, &b, IMM),        \
                TEST_NAME " predicate " #IMM " mask 0x%x", result);          \
} while (0)

int main(void) {
    zmm_test_t a, b;
    uint32_t result, writemask;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;
    for (int i = 0; i < TEST_LANES; i++) {
        a.TEST_FIELD[i] = (test_float_t)i;
        b.TEST_FIELD[i] = (test_float_t)i;
    }
    a.TEST_FIELD[0] = NAN;
    b.TEST_FIELD[1] = NAN;
    a.TEST_FIELD[2] = -INFINITY;
    b.TEST_FIELD[2] = INFINITY;
    a.TEST_FIELD[3] = (test_float_t)2;
    b.TEST_FIELD[3] = (test_float_t)1;
    a.TEST_FIELD[TEST_LANES - 1] = (test_float_t)-0.0;
    b.TEST_FIELD[TEST_LANES - 1] = (test_float_t)0.0;

    RUN_COMPARE(0);
    RUN_COMPARE(1);
    RUN_COMPARE(3);
    RUN_COMPARE(4);
    RUN_COMPARE(7);

    writemask = (UINT32_C(1) << 0) | (UINT32_C(1) << 3) |
                (UINT32_C(1) << (TEST_LANES - 1));
    __asm__ volatile("kmovw %3, %%k2\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     TEST_INSN " $0, %%zmm1, %%zmm0, %%k1%{%%k2%}\n\t"
                     TEST_KMOV_OUT " %%k1, %0"
                     : "=r"(result)
                     : "m"(a), "m"(b), "r"(writemask)
                     : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT((result & TEST_ALL_MASK) ==
                (expected_mask(&a, &b, 0) & writemask),
                TEST_NAME " destination writemask");
    TEST_END();
}

#undef RUN_COMPARE
#endif
