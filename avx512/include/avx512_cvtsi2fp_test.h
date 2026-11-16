#ifndef X86_64_AVX512_CVTSI2FP_TEST_H
#define X86_64_AVX512_CVTSI2FP_TEST_H

#include "avx512_test.h"
#include <limits.h>

static void convert_i64(int64_t input, const xmm_t *upper, xmm_t *out) {
    __asm__ volatile("vmovdqa64 %1, %%xmm16\n\t"
                     TEST_INSN " %2, %%xmm16, %%xmm17\n\t"
                     "vmovdqa64 %%xmm17, %0"
                     : "=m"(*out) : "m"(*upper), "r"(input)
                     : "xmm16", "xmm17");
}

static void convert_i32(int32_t input, const xmm_t *upper, xmm_t *out) {
    __asm__ volatile("vmovdqa64 %1, %%xmm16\n\t"
                     TEST_INSN " %2, %%xmm16, %%xmm17\n\t"
                     "vmovdqa64 %%xmm17, %0"
                     : "=m"(*out) : "m"(*upper), "r"(input)
                     : "xmm16", "xmm17");
}

static void check_upper(const xmm_t *out, const xmm_t *upper) {
#if TEST_BITS == 32
    for (int i = 1; i < 4; i++)
        TEST_ASSERT(out->u32[i] == upper->u32[i],
                    TEST_NAME " preserves upper dword %d", i);
#else
    TEST_ASSERT(out->u64[1] == upper->u64[1],
                TEST_NAME " preserves upper qword");
#endif
}

int main(void) {
    xmm_t upper, out;
    upper.u64[0] = UINT64_C(0x1122334455667788);
    upper.u64[1] = UINT64_C(0x99aabbccddeeff00);

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;

    convert_i32(INT32_MIN, &upper, &out);
    TEST_ASSERT(out.TEST_FIELD[0] == (TEST_FLOAT_T)INT32_MIN,
                TEST_NAME " INT32_MIN");
    check_upper(&out, &upper);
    convert_i32(INT32_MAX, &upper, &out);
    TEST_ASSERT(out.TEST_FIELD[0] == (TEST_FLOAT_T)INT32_MAX,
                TEST_NAME " INT32_MAX");
    check_upper(&out, &upper);

    convert_i64(INT64_MIN, &upper, &out);
    TEST_ASSERT(out.TEST_FIELD[0] == -TEST_TWO63,
                TEST_NAME " INT64_MIN");
    check_upper(&out, &upper);
    convert_i64(INT64_MAX, &upper, &out);
    TEST_ASSERT(out.TEST_FIELD[0] == TEST_TWO63,
                TEST_NAME " INT64_MAX rounds to 2^63");
    check_upper(&out, &upper);
    convert_i64(TEST_ROUND_INPUT, &upper, &out);
    TEST_ASSERT(out.TEST_FIELD[0] == TEST_ROUND_EXPECTED,
                TEST_NAME " nearest ties-to-even boundary");
    check_upper(&out, &upper);
    TEST_END();
}

#endif
