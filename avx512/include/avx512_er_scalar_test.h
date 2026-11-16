#ifndef X86_64_AVX512_ER_SCALAR_TEST_H
#define X86_64_AVX512_ER_SCALAR_TEST_H

#include "avx512_test.h"
#include <math.h>

#if TEST_BITS == 32
#define TEST_FIELD f32
#define TEST_UINT_FIELD u32
#define TEST_LANES 4
#define TEST_KMOV "kmovw"
#define TEST_SENTINEL_BITS UINT32_C(0x4a40e400)
typedef float test_float_t;
typedef uint32_t test_uint_t;
#elif TEST_BITS == 64
#define TEST_FIELD f64
#define TEST_UINT_FIELD u64
#define TEST_LANES 2
#define TEST_KMOV "kmovb"
#define TEST_SENTINEL_BITS UINT64_C(0x40c81c8000000000)
typedef double test_float_t;
typedef uint64_t test_uint_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

static int test_close(test_float_t actual, test_float_t expected) {
    if (isinf(expected))
        return isinf(actual) && signbit(actual) == signbit(expected);
    if (expected == (test_float_t)0)
        return actual == expected && signbit(actual) == signbit(expected);
    return fabs((double)(actual - expected)) <=
           fabs((double)expected) * TEST_TOLERANCE;
}

static void check_upper_lanes(const xmm_t *out, const xmm_t *upper_source) {
    for (int i = 1; i < TEST_LANES; i++)
        TEST_ASSERT(out->TEST_UINT_FIELD[i] == upper_source->TEST_UINT_FIELD[i],
                    TEST_NAME " upper source lane %d", i);
}

static void run_case(test_float_t input, test_float_t expected) {
    xmm_t upper_source, scalar_source, initial, out;
    uint32_t mask;

    for (int i = 0; i < TEST_LANES; i++) {
        upper_source.TEST_UINT_FIELD[i] =
            (test_uint_t)(TEST_SENTINEL_BITS ^ (test_uint_t)(i * 0x11111111U));
        scalar_source.TEST_UINT_FIELD[i] =
            (test_uint_t)(TEST_SENTINEL_BITS ^ (test_uint_t)(i * 0x01010101U));
        initial.TEST_UINT_FIELD[i] = TEST_SENTINEL_BITS;
    }
    scalar_source.TEST_FIELD[0] = input;

    __asm__ volatile("vmovdqa %1, %%xmm0\n\tvmovdqa %2, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\t"
                     "vmovdqa %%xmm2, %0"
                     : "=m"(out)
                     : "m"(upper_source), "m"(scalar_source)
                     : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(test_close(out.TEST_FIELD[0], expected),
                TEST_NAME " result: got %a expected %a",
                (double)out.TEST_FIELD[0], (double)expected);
    check_upper_lanes(&out, &upper_source);

    mask = 0;
    __asm__ volatile(TEST_KMOV " %4, %%k1\n\t"
                     "vmovdqa %1, %%xmm0\n\tvmovdqa %2, %%xmm1\n\t"
                     "vmovdqa %3, %%xmm2\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2%{%%k1%}\n\t"
                     "vmovdqa %%xmm2, %0"
                     : "=m"(out)
                     : "m"(upper_source), "m"(scalar_source), "m"(initial),
                       "r"(mask)
                     : "xmm0", "xmm1", "xmm2", "k1");
    TEST_ASSERT(out.TEST_UINT_FIELD[0] == TEST_SENTINEL_BITS,
                TEST_NAME " empty merge mask preserves destination");
    check_upper_lanes(&out, &upper_source);

    mask = 2;
    __asm__ volatile(TEST_KMOV " %3, %%k1\n\t"
                     "vmovdqa %1, %%xmm0\n\tvmovdqa %2, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2%{%%k1%}%{z%}\n\t"
                     "vmovdqa %%xmm2, %0"
                     : "=m"(out)
                     : "m"(upper_source), "m"(scalar_source), "r"(mask)
                     : "xmm0", "xmm1", "xmm2", "k1");
    TEST_ASSERT(out.TEST_UINT_FIELD[0] == 0,
                TEST_NAME " high-only zero mask clears scalar lane");
    check_upper_lanes(&out, &upper_source);
}

int main(void) {
    TEST_START(TEST_NAME);
    if (!CHECK_AVX512ER()) return 0;
    run_case((test_float_t)3, TEST_REFERENCE((test_float_t)3));
    run_case((test_float_t)0, INFINITY);
    run_case(INFINITY, (test_float_t)0);
    TEST_END();
}

#endif
