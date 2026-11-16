#ifndef X86_64_AVX512_VNNI_DOT_TEST_H
#define X86_64_AVX512_VNNI_DOT_TEST_H

#include "avx512_test.h"
#include <limits.h>

#if TEST_SATURATING
static int32_t clamp_i64(int64_t value) {
    if (value > INT32_MAX) return INT32_MAX;
    if (value < INT32_MIN) return INT32_MIN;
    return (int32_t)value;
}
#endif

static int64_t dot_sum(const zmm_test_t *acc, const zmm_test_t *a,
                       const zmm_test_t *b, int lane) {
    int64_t sum = acc->i32[lane];
#if TEST_BYTE_DOT
    for (int i = 0; i < 4; i++)
        sum += (int64_t)a->u8[lane * 4 + i] * b->i8[lane * 4 + i];
#else
    for (int i = 0; i < 2; i++)
        sum += (int64_t)a->i16[lane * 2 + i] * b->i16[lane * 2 + i];
#endif
    return sum;
}

static uint32_t expected_bits(const zmm_test_t *acc, const zmm_test_t *a,
                              const zmm_test_t *b, int lane) {
    int64_t sum = dot_sum(acc, a, b, lane);
#if TEST_SATURATING
    return (uint32_t)clamp_i64(sum);
#else
    return (uint32_t)sum;
#endif
}

static void check_result(const zmm_test_t *out, const zmm_test_t *acc,
                         const zmm_test_t *a, const zmm_test_t *b,
                         uint32_t mask, int zeroing) {
    for (int lane = 0; lane < 16; lane++) {
        uint32_t expected;
        if ((mask >> lane) & 1U)
            expected = expected_bits(acc, a, b, lane);
        else
            expected = zeroing ? 0 : acc->u32[lane];
        TEST_ASSERT(out->u32[lane] == expected,
                    TEST_NAME " lane %d: 0x%08x != 0x%08x", lane,
                    out->u32[lane], expected);
    }
}

#if TEST_SATURATING
static void test_saturation(int positive) {
    zmm_test_t acc, a, b, out;
    for (int lane = 0; lane < 16; lane++)
        acc.i32[lane] = positive ? INT32_MAX - 1 : INT32_MIN + 1;
#if TEST_BYTE_DOT
    for (int i = 0; i < 64; i++) {
        a.u8[i] = UINT8_MAX;
        b.i8[i] = positive ? INT8_MAX : INT8_MIN;
    }
#else
    for (int i = 0; i < 32; i++) {
        a.i16[i] = INT16_MAX;
        b.i16[i] = positive ? INT16_MAX : INT16_MIN;
    }
#endif
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vmovdqa64 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out) : "m"(acc), "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2");
    for (int lane = 0; lane < 16; lane++)
        TEST_ASSERT(out.i32[lane] == (positive ? INT32_MAX : INT32_MIN),
                    TEST_NAME " %s saturation lane %d",
                    positive ? "positive" : "negative", lane);
}
#endif

int main(void) {
    zmm_test_t acc, a, b, out;
    uint32_t mask;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512VNNI()) return 0;
    for (int lane = 0; lane < 16; lane++) acc.i32[lane] = lane * 100 - 700;
#if TEST_BYTE_DOT
    for (int i = 0; i < 64; i++) {
        a.u8[i] = (uint8_t)(i * 13U + 1U);
        b.i8[i] = (int8_t)(i * 7 - 100);
    }
#else
    for (int i = 0; i < 32; i++) {
        a.i16[i] = (int16_t)(i * 997 - 15000);
        b.i16[i] = (int16_t)(20000 - i * 733);
    }
#endif

    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vmovdqa64 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out) : "m"(acc), "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2");
    check_result(&out, &acc, &a, &b, UINT32_C(0xffff), 0);

    mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %4, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(acc), "m"(a), "m"(b), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    check_result(&out, &acc, &a, &b, mask, 0);

    mask = UINT32_C(0xa55a);
    __asm__ volatile("kmovw %4, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(acc), "m"(a), "m"(b), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    check_result(&out, &acc, &a, &b, mask, 1);

#if TEST_SATURATING
    test_saturation(1);
    test_saturation(0);
#endif
    TEST_END();
}

#endif
