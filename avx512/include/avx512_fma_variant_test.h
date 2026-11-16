#ifndef X86_64_AVX512_FMA_VARIANT_TEST_H
#define X86_64_AVX512_FMA_VARIANT_TEST_H

#include "avx512_test.h"
#include <math.h>

#define TEST_KIND_ADDSUB 1
#define TEST_KIND_SUBADD 2
#define TEST_KIND_FNMADD 3
#define TEST_KIND_FNMSUB 4

#if TEST_BITS == 32
#define TEST_FIELD f32
#define TEST_UINT_FIELD u32
#define TEST_LANES 16
#define TEST_KMOV "kmovw"
#define TEST_ALL_MASK UINT32_C(0xffff)
#define TEST_ENDPOINT_MASK UINT32_C(0x8001)
#define TEST_MIXED_MASK UINT32_C(0x5aa5)
#define TEST_X 0x1.000002p+0f
#define TEST_Z 0x1.000004p+0f
typedef float test_float_t;
static test_float_t test_fma(test_float_t a, test_float_t b, test_float_t c) {
    return fmaf(a, b, c);
}
#elif TEST_BITS == 64
#define TEST_FIELD f64
#define TEST_UINT_FIELD u64
#define TEST_LANES 8
#define TEST_KMOV "kmovb"
#define TEST_ALL_MASK UINT32_C(0xff)
#define TEST_ENDPOINT_MASK UINT32_C(0x81)
#define TEST_MIXED_MASK UINT32_C(0x5a)
#define TEST_X 0x1.0000000000001p+0
#define TEST_Z 0x1.0000000000002p+0
typedef double test_float_t;
static test_float_t test_fma(test_float_t a, test_float_t b, test_float_t c) {
    return fma(a, b, c);
}
#else
#error "TEST_BITS must be 32 or 64"
#endif

static void set_roles(zmm_test_t *a, zmm_test_t *b, zmm_test_t *c,
                      int lane, test_float_t product_a,
                      test_float_t product_b, test_float_t addend) {
#if TEST_FORM == 132
    a->TEST_FIELD[lane] = product_a;
    c->TEST_FIELD[lane] = product_b;
    b->TEST_FIELD[lane] = addend;
#elif TEST_FORM == 213
    a->TEST_FIELD[lane] = product_a;
    b->TEST_FIELD[lane] = product_b;
    c->TEST_FIELD[lane] = addend;
#elif TEST_FORM == 231
    b->TEST_FIELD[lane] = product_a;
    c->TEST_FIELD[lane] = product_b;
    a->TEST_FIELD[lane] = addend;
#else
#error "TEST_FORM must be 132, 213, or 231"
#endif
}

static void get_roles(const zmm_test_t *a, const zmm_test_t *b,
                      const zmm_test_t *c, int lane, test_float_t *product_a,
                      test_float_t *product_b, test_float_t *addend) {
#if TEST_FORM == 132
    *product_a = a->TEST_FIELD[lane];
    *product_b = c->TEST_FIELD[lane];
    *addend = b->TEST_FIELD[lane];
#elif TEST_FORM == 213
    *product_a = a->TEST_FIELD[lane];
    *product_b = b->TEST_FIELD[lane];
    *addend = c->TEST_FIELD[lane];
#else
    *product_a = b->TEST_FIELD[lane];
    *product_b = c->TEST_FIELD[lane];
    *addend = a->TEST_FIELD[lane];
#endif
}

static test_float_t expected_lane(const zmm_test_t *a, const zmm_test_t *b,
                                  const zmm_test_t *c, int lane) {
    test_float_t product_a, product_b, addend;
    get_roles(a, b, c, lane, &product_a, &product_b, &addend);
#if TEST_KIND == TEST_KIND_ADDSUB
    return test_fma(product_a, product_b, (lane & 1) ? addend : -addend);
#elif TEST_KIND == TEST_KIND_SUBADD
    return test_fma(product_a, product_b, (lane & 1) ? -addend : addend);
#elif TEST_KIND == TEST_KIND_FNMADD
    return test_fma(-product_a, product_b, addend);
#elif TEST_KIND == TEST_KIND_FNMSUB
    return test_fma(-product_a, product_b, -addend);
#else
#error "Unknown TEST_KIND"
#endif
}

static void check_result(const zmm_test_t *out, const zmm_test_t *a,
                         const zmm_test_t *b, const zmm_test_t *c,
                         uint32_t mask, int zeroing) {
    for (int lane = 0; lane < TEST_LANES; lane++) {
        test_float_t expected = (mask >> lane) & 1U
            ? expected_lane(a, b, c, lane)
            : (zeroing ? (test_float_t)0 : a->TEST_FIELD[lane]);
        TEST_ASSERT(out->TEST_FIELD[lane] == expected,
                    TEST_NAME " lane %d: got %a expected %a", lane,
                    (double)out->TEST_FIELD[lane], (double)expected);
    }
}

static void check_fused_discriminator(const zmm_test_t *a,
                                      const zmm_test_t *b,
                                      const zmm_test_t *c) {
    test_float_t product_a, product_b, addend;
    get_roles(a, b, c, 0, &product_a, &product_b, &addend);
    volatile test_float_t rounded_product = product_a * product_b;
    test_float_t separate;
#if TEST_KIND == TEST_KIND_ADDSUB
    separate = rounded_product - addend;
#elif TEST_KIND == TEST_KIND_SUBADD
    separate = rounded_product + addend;
#elif TEST_KIND == TEST_KIND_FNMADD
    separate = -rounded_product + addend;
#else
    separate = -rounded_product - addend;
#endif
    TEST_ASSERT(separate == (test_float_t)0,
                TEST_NAME " discriminator separate result is zero");
    TEST_ASSERT(expected_lane(a, b, c, 0) != separate,
                TEST_NAME " discriminator requires fused rounding");
}

int main(void) {
    zmm_test_t a, b, c, out;
    uint32_t mask;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;
    for (int lane = 0; lane < TEST_LANES; lane++) {
        test_float_t product_a = (test_float_t)(lane + 1);
        test_float_t product_b = (test_float_t)2;
        test_float_t addend = (test_float_t)(lane & 3) * (test_float_t)0.25;
        set_roles(&a, &b, &c, lane, product_a, product_b, addend);
    }

#if TEST_KIND == TEST_KIND_SUBADD || TEST_KIND == TEST_KIND_FNMSUB
    set_roles(&a, &b, &c, 0, TEST_X, TEST_X, -TEST_Z);
#else
    set_roles(&a, &b, &c, 0, TEST_X, TEST_X, TEST_Z);
#endif
    check_fused_discriminator(&a, &b, &c);

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vmovdqa64 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2");
    check_result(&out, &a, &b, &c, TEST_ALL_MASK, 0);

    mask = TEST_ENDPOINT_MASK;
    __asm__ volatile(TEST_KMOV " %4, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(a), "m"(b), "m"(c), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    check_result(&out, &a, &b, &c, mask, 0);

    mask = TEST_MIXED_MASK;
    __asm__ volatile(TEST_KMOV " %4, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(a), "m"(b), "m"(c), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    check_result(&out, &a, &b, &c, mask, 1);
    TEST_END();
}

#endif
