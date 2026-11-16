#ifndef X86_64_AVX512_CMP_SCALAR_TEST_H
#define X86_64_AVX512_CMP_SCALAR_TEST_H

#include "avx512_test.h"
#include <math.h>

#if TEST_BITS == 32
#define TEST_FIELD f32
typedef float test_float_t;
#elif TEST_BITS == 64
#define TEST_FIELD f64
typedef double test_float_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

static uint32_t run_compare(test_float_t a, test_float_t b, int predicate) {
    xmm_t av = {0}, bv = {0};
    uint32_t result = 0;
    av.TEST_FIELD[0] = a;
    bv.TEST_FIELD[0] = b;
    switch (predicate) {
        case 0:
            __asm__ volatile("vmovdqa64 %1, %%xmm0\n\tvmovdqa64 %2, %%xmm1\n\t"
                             TEST_INSN " $0, %%xmm1, %%xmm0, %%k1\n\t"
                             "kmovb %%k1, %0"
                             : "=r"(result) : "m"(av), "m"(bv)
                             : "xmm0", "xmm1", "k1");
            break;
        case 1:
            __asm__ volatile("vmovdqa64 %1, %%xmm0\n\tvmovdqa64 %2, %%xmm1\n\t"
                             TEST_INSN " $1, %%xmm1, %%xmm0, %%k1\n\t"
                             "kmovb %%k1, %0"
                             : "=r"(result) : "m"(av), "m"(bv)
                             : "xmm0", "xmm1", "k1");
            break;
        default:
            __asm__ volatile("vmovdqa64 %1, %%xmm0\n\tvmovdqa64 %2, %%xmm1\n\t"
                             TEST_INSN " $3, %%xmm1, %%xmm0, %%k1\n\t"
                             "kmovb %%k1, %0"
                             : "=r"(result) : "m"(av), "m"(bv)
                             : "xmm0", "xmm1", "k1");
            break;
    }
    return result & 1U;
}

int main(void) {
    xmm_t a = {0}, b = {0};
    uint32_t result, zero = 0;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;
    TEST_ASSERT(run_compare(NAN, (test_float_t)1, 3) == 1,
                TEST_NAME " unordered NaN");
    TEST_ASSERT(run_compare((test_float_t)-0.0, (test_float_t)0.0, 0) == 1,
                TEST_NAME " signed zeros compare equal");
    TEST_ASSERT(run_compare((test_float_t)-2, (test_float_t)1, 1) == 1,
                TEST_NAME " less-than predicate");
    TEST_ASSERT(run_compare((test_float_t)2, (test_float_t)1, 1) == 0,
                TEST_NAME " false less-than predicate");

    a.TEST_FIELD[0] = (test_float_t)1;
    b.TEST_FIELD[0] = (test_float_t)1;
    __asm__ volatile("kmovb %3, %%k2\n\tvmovdqa64 %1, %%xmm0\n\t"
                     "vmovdqa64 %2, %%xmm1\n\t"
                     TEST_INSN " $0, %%xmm1, %%xmm0, %%k1%{%%k2%}\n\t"
                     "kmovb %%k1, %0"
                     : "=r"(result) : "m"(a), "m"(b), "r"(zero)
                     : "xmm0", "xmm1", "k1", "k2");
    TEST_ASSERT((result & 1U) == 0, TEST_NAME " empty destination writemask");
    TEST_END();
}

#endif
