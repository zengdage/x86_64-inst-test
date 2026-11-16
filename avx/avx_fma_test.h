#ifndef X86_64_AVX_FMA_TEST_H
#define X86_64_AVX_FMA_TEST_H

#include "../common.h"
#include <math.h>

#define FMA_KIND_ADD 1
#define FMA_KIND_SUB 2
#define FMA_KIND_ADDSUB 3
#define FMA_KIND_SUBADD 4
#define FMA_KIND_NMADD 5
#define FMA_KIND_NMSUB 6

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_fma(void) {
    uint32_t eax, ebx, ecx, edx;
    uint32_t xcr0_lo, xcr0_hi;

    __asm__("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(1), "c"(0));
    if ((ecx & ((1U << 12) | (1U << 27) | (1U << 28))) !=
        ((1U << 12) | (1U << 27) | (1U << 28)))
        return 0;
    __asm__("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
    return (xcr0_lo & 0x6U) == 0x6U;
}
#else
#define check_fma() 1
#endif

#if TEST_BITS == 32
typedef float test_float_t;
#define TEST_FIELD f32
#define TEST_UINT_FIELD u32
#define TEST_PACKED_LANES 8
#define TEST_XMM_LANES 4
#define TEST_X 0x1.000002p+0f
#define TEST_Z 0x1.000004p+0f
#define TEST_FMA fmaf
#elif TEST_BITS == 64
typedef double test_float_t;
#define TEST_FIELD f64
#define TEST_UINT_FIELD u64
#define TEST_PACKED_LANES 4
#define TEST_XMM_LANES 2
#define TEST_X 0x1.0000000000001p+0
#define TEST_Z 0x1.0000000000002p+0
#define TEST_FMA fma
#else
#error "TEST_BITS must be 32 or 64"
#endif

static void set_roles(ymm_t *a, ymm_t *b, ymm_t *c, int lane,
                      test_float_t product_a, test_float_t product_b,
                      test_float_t addend) {
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

static void get_roles(const ymm_t *a, const ymm_t *b, const ymm_t *c,
                      int lane, test_float_t *product_a,
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

static test_float_t expected_lane(const ymm_t *a, const ymm_t *b,
                                  const ymm_t *c, int lane) {
    test_float_t product_a, product_b, addend;
    get_roles(a, b, c, lane, &product_a, &product_b, &addend);
#if TEST_KIND == FMA_KIND_ADD
    return TEST_FMA(product_a, product_b, addend);
#elif TEST_KIND == FMA_KIND_SUB
    return TEST_FMA(product_a, product_b, -addend);
#elif TEST_KIND == FMA_KIND_ADDSUB
    return TEST_FMA(product_a, product_b, (lane & 1) ? addend : -addend);
#elif TEST_KIND == FMA_KIND_SUBADD
    return TEST_FMA(product_a, product_b, (lane & 1) ? -addend : addend);
#elif TEST_KIND == FMA_KIND_NMADD
    return TEST_FMA(-product_a, product_b, addend);
#elif TEST_KIND == FMA_KIND_NMSUB
    return TEST_FMA(-product_a, product_b, -addend);
#else
#error "Unknown TEST_KIND"
#endif
}

static test_float_t discriminator_addend(void) {
#if TEST_KIND == FMA_KIND_ADD
    return -TEST_Z;
#elif TEST_KIND == FMA_KIND_SUB || TEST_KIND == FMA_KIND_ADDSUB
    return TEST_Z;
#elif TEST_KIND == FMA_KIND_SUBADD || TEST_KIND == FMA_KIND_NMSUB
    return -TEST_Z;
#else
    return TEST_Z;
#endif
}

static void init_operands(ymm_t *a, ymm_t *b, ymm_t *c) {
    memset(a, 0, sizeof(*a));
    memset(b, 0, sizeof(*b));
    memset(c, 0, sizeof(*c));
    for (int lane = 0; lane < TEST_PACKED_LANES; lane++)
        set_roles(a, b, c, lane, (test_float_t)(lane + 2),
                  (test_float_t)(lane + 3),
                  (test_float_t)((lane & 3) + 1));
    set_roles(a, b, c, 0, TEST_X, TEST_X, discriminator_addend());
}

static void check_fused_discriminator(const ymm_t *a, const ymm_t *b,
                                      const ymm_t *c) {
    test_float_t product_a, product_b, addend, separate;
    volatile test_float_t rounded_product;
    get_roles(a, b, c, 0, &product_a, &product_b, &addend);
    rounded_product = product_a * product_b;
#if TEST_KIND == FMA_KIND_ADD
    separate = rounded_product + addend;
#elif TEST_KIND == FMA_KIND_SUB || TEST_KIND == FMA_KIND_ADDSUB
    separate = rounded_product - addend;
#elif TEST_KIND == FMA_KIND_SUBADD
    separate = rounded_product + addend;
#elif TEST_KIND == FMA_KIND_NMADD
    separate = -rounded_product + addend;
#else
    separate = -rounded_product - addend;
#endif
    TEST_ASSERT(expected_lane(a, b, c, 0) != separate,
                TEST_NAME " must use fused single rounding");
}

#if TEST_SCALAR
static int run_test(void) {
    ymm_t a, b, c, out;
    init_operands(&a, &b, &c);
    for (int lane = 1; lane < TEST_XMM_LANES; lane++)
        a.TEST_UINT_FIELD[lane] =
#if TEST_BITS == 32
            UINT32_C(0x41000000) + (uint32_t)lane;
#else
            UINT64_C(0x4020000000000000) + (uint64_t)lane;
#endif
    memset(&out, 0xa5, sizeof(out));
    check_fused_discriminator(&a, &b, &c);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     "movdqu %2, %%xmm0\n\t"
                     "movdqu %3, %%xmm1\n\t"
                     "movdqu %4, %%xmm2\n\t"
                     TEST_INSN " %%xmm2, %%xmm1, %%xmm0\n\t"
                     "vmovdqu %%ymm0, %0"
                     : "=m"(out)
                     : "m"(out), "m"(a), "m"(b), "m"(c)
                     : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(out.TEST_FIELD[0] == expected_lane(&a, &b, &c, 0),
                TEST_NAME " scalar result: got %a expected %a",
                (double)out.TEST_FIELD[0],
                (double)expected_lane(&a, &b, &c, 0));
    for (int lane = 1; lane < TEST_XMM_LANES; lane++)
        TEST_ASSERT(out.TEST_UINT_FIELD[lane] == a.TEST_UINT_FIELD[lane],
                    TEST_NAME " preserves XMM upper lane %d", lane);
    for (int lane = TEST_XMM_LANES; lane < TEST_PACKED_LANES; lane++)
        TEST_ASSERT(out.TEST_UINT_FIELD[lane] == 0,
                    TEST_NAME " zeroes YMM upper lane %d", lane);

    __asm__ volatile("movdqu %1, %%xmm0\n\t"
                     "movdqu %2, %%xmm1\n\t"
                     TEST_INSN " %3, %%xmm1, %%xmm0\n\t"
                     "movdqu %%xmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "xmm0", "xmm1");
    TEST_ASSERT(out.TEST_FIELD[0] == expected_lane(&a, &b, &c, 0),
                TEST_NAME " scalar memory source");
    return 0;
}
#else
static void check_lanes(const ymm_t *out, const ymm_t *a, const ymm_t *b,
                        const ymm_t *c, int lanes) {
    for (int lane = 0; lane < lanes; lane++)
        TEST_ASSERT(out->TEST_FIELD[lane] == expected_lane(a, b, c, lane),
                    TEST_NAME " lane %d: got %a expected %a", lane,
                    (double)out->TEST_FIELD[lane],
                    (double)expected_lane(a, b, c, lane));
}

static int run_test(void) {
    ymm_t a, b, c, out, initial;
    init_operands(&a, &b, &c);
    check_fused_discriminator(&a, &b, &c);

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vmovdqu %3, %%ymm2\n\t"
                     TEST_INSN " %%ymm2, %%ymm1, %%ymm0\n\t"
                     "vmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "ymm0", "ymm1", "ymm2");
    check_lanes(&out, &a, &b, &c, TEST_PACKED_LANES);

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " %3, %%ymm1, %%ymm0\n\t"
                     "vmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "ymm0", "ymm1");
    check_lanes(&out, &a, &b, &c, TEST_PACKED_LANES);

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     "movdqu %2, %%xmm0\n\tmovdqu %3, %%xmm1\n\t"
                     "movdqu %4, %%xmm2\n\t"
                     TEST_INSN " %%xmm2, %%xmm1, %%xmm0\n\t"
                     "vmovdqu %%ymm0, %0"
                     : "=m"(out)
                     : "m"(initial), "m"(a), "m"(b), "m"(c)
                     : "xmm0", "xmm1", "xmm2");
    check_lanes(&out, &a, &b, &c, TEST_XMM_LANES);
    for (int lane = TEST_XMM_LANES; lane < TEST_PACKED_LANES; lane++)
        TEST_ASSERT(out.TEST_UINT_FIELD[lane] == 0,
                    TEST_NAME " VEX.128 zeroes upper lane %d", lane);
    return 0;
}
#endif

int main(void) {
    TEST_START(TEST_NAME);
    if (!check_fma()) return 0;
    run_test();
    TEST_END();
}

#endif
