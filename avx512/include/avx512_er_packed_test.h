#ifndef X86_64_AVX512_ER_PACKED_TEST_H
#define X86_64_AVX512_ER_PACKED_TEST_H

#include "avx512_test.h"
#include <math.h>

#if TEST_BITS == 32
#define TEST_FIELD f32
#define TEST_UINT_FIELD u32
#define TEST_LANES 16
#define TEST_KMOV "kmovw"
#define TEST_ALL_MASK UINT32_C(0xffff)
#define TEST_ENDPOINT_MASK UINT32_C(0x8001)
#define TEST_MIXED_MASK UINT32_C(0xa55a)
#define TEST_SENTINEL 12345.0f
typedef float test_float_t;
#elif TEST_BITS == 64
#define TEST_FIELD f64
#define TEST_UINT_FIELD u64
#define TEST_LANES 8
#define TEST_KMOV "kmovb"
#define TEST_ALL_MASK UINT32_C(0xff)
#define TEST_ENDPOINT_MASK UINT32_C(0x81)
#define TEST_MIXED_MASK UINT32_C(0x5a)
#define TEST_SENTINEL 12345.0
typedef double test_float_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

static int test_close(test_float_t actual, test_float_t expected) {
    if (isnan(expected))
        return isnan(actual);
    if (isinf(expected))
        return isinf(actual) && signbit(actual) == signbit(expected);
    if (expected == (test_float_t)0)
        return actual == expected && signbit(actual) == signbit(expected);
    return fabs((double)(actual - expected)) <=
           fabs((double)expected) * TEST_TOLERANCE;
}

static void check_result(const zmm_test_t *out, const zmm_test_t *src,
                         uint32_t mask, int zeroing) {
    for (int i = 0; i < TEST_LANES; i++) {
        test_float_t expected;
        if ((mask >> i) & 1U)
            expected = TEST_REFERENCE(src->TEST_FIELD[i]);
        else
            expected = zeroing ? (test_float_t)0 : TEST_SENTINEL;
        TEST_ASSERT(test_close(out->TEST_FIELD[i], expected),
                    TEST_NAME " lane %d: got %a expected %a", i,
                    (double)out->TEST_FIELD[i], (double)expected);
    }
}

int main(void) {
    static const double inputs[8] = {
        0.0, INFINITY, 0.25, 0.5, 1.0, 2.0, 3.0, -1.0
    };
    zmm_test_t src, initial, out;
    uint32_t mask;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512ER()) return 0;

    for (int i = 0; i < TEST_LANES; i++) {
        src.TEST_FIELD[i] = (test_float_t)inputs[i & 7];
        initial.TEST_FIELD[i] = TEST_SENTINEL;
    }

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\t"
                     TEST_INSN " %%zmm0, %%zmm1\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    check_result(&out, &src, TEST_ALL_MASK, 0);

    mask = TEST_ENDPOINT_MASK;
    __asm__ volatile(TEST_KMOV " %3, %%k1\n\t"
                     "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     TEST_INSN " %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 0);

    mask = TEST_MIXED_MASK;
    __asm__ volatile(TEST_KMOV " %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     TEST_INSN " %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 1);

    mask = 0;
    __asm__ volatile(TEST_KMOV " %3, %%k1\n\t"
                     "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     TEST_INSN " %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 0);

    TEST_END();
}

#endif
