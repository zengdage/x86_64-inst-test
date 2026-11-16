#ifndef X86_64_AVX_FP_BINARY_TEST_H
#define X86_64_AVX_FP_BINARY_TEST_H

#include "../common.h"
#include <math.h>

#define FP_OP_ADD 1
#define FP_OP_SUB 2
#define FP_OP_MUL 3
#define FP_OP_DIV 4
#define FP_OP_MAX 5
#define FP_OP_MIN 6
#define FP_OP_ADDSUB 7

#if TEST_BITS == 32
typedef float fp_t;
#define FP_FIELD f32
#define UINT_FIELD u32
#define UINT_TYPE uint32_t
#define PACKED_LANES 8
#define XMM_LANES 4
#define SIGN_MASK UINT32_C(0x80000000)
#elif TEST_BITS == 64
typedef double fp_t;
#define FP_FIELD f64
#define UINT_FIELD u64
#define UINT_TYPE uint64_t
#define PACKED_LANES 4
#define XMM_LANES 2
#define SIGN_MASK UINT64_C(0x8000000000000000)
#else
#error "TEST_BITS must be 32 or 64"
#endif

static fp_t expected_value(fp_t a, fp_t b, int lane) {
#if TEST_OP == FP_OP_ADD
    return a + b;
#elif TEST_OP == FP_OP_SUB
    return a - b;
#elif TEST_OP == FP_OP_MUL
    return a * b;
#elif TEST_OP == FP_OP_DIV
    return a / b;
#elif TEST_OP == FP_OP_MAX
    return a > b ? a : b;
#elif TEST_OP == FP_OP_MIN
    return a < b ? a : b;
#elif TEST_OP == FP_OP_ADDSUB
    return (lane & 1) ? a + b : a - b;
#else
#error "Unknown TEST_OP"
#endif
}

static void init_values(ymm_t *a, ymm_t *b) {
    memset(a, 0, sizeof(*a));
    memset(b, 0, sizeof(*b));
    for (int i = 0; i < PACKED_LANES; i++) {
        a->FP_FIELD[i] = (fp_t)(i + 2) * (i & 1 ? (fp_t)-1 : (fp_t)1);
        b->FP_FIELD[i] = (fp_t)((i % 3) + 1) * (i & 2 ? (fp_t)-1 : (fp_t)1);
    }
}

#if !TEST_SCALAR
static void check_packed(const ymm_t *out, const ymm_t *a, const ymm_t *b,
                         int lanes, const char *form) {
    for (int i = 0; i < lanes; i++) {
        fp_t expected = expected_value(a->FP_FIELD[i], b->FP_FIELD[i], i);
        TEST_ASSERT(out->FP_FIELD[i] == expected,
                    TEST_NAME " %s lane %d got %a expected %a", form, i,
                    (double)out->FP_FIELD[i], (double)expected);
    }
}
#endif

static void test_special_values(void) {
    ymm_t a, b, out;
    init_values(&a, &b);
#if TEST_OP == FP_OP_MAX || TEST_OP == FP_OP_MIN
    a.FP_FIELD[0] = (fp_t)0.0;
    b.FP_FIELD[0] = (fp_t)-0.0;
    a.FP_FIELD[1] = (fp_t)NAN;
    b.FP_FIELD[1] = (fp_t)7.0;
    a.FP_FIELD[2] = (fp_t)-3.0;
    b.UINT_FIELD[2] =
#if TEST_BITS == 32
        UINT32_C(0x7fc12345);
#else
        UINT64_C(0x7ff8123456789abc);
#endif
#elif TEST_OP == FP_OP_ADD
    a.FP_FIELD[0] = (fp_t)INFINITY;
    b.FP_FIELD[0] = (fp_t)-INFINITY;
    a.FP_FIELD[1] = (fp_t)-0.0;
    b.FP_FIELD[1] = (fp_t)-0.0;
#elif TEST_OP == FP_OP_SUB
    a.FP_FIELD[0] = (fp_t)INFINITY;
    b.FP_FIELD[0] = (fp_t)INFINITY;
    a.FP_FIELD[1] = (fp_t)-0.0;
    b.FP_FIELD[1] = (fp_t)0.0;
#elif TEST_OP == FP_OP_MUL
    a.FP_FIELD[0] = (fp_t)INFINITY;
    b.FP_FIELD[0] = (fp_t)0.0;
    a.FP_FIELD[1] = (fp_t)-0.0;
    b.FP_FIELD[1] = (fp_t)3.0;
#elif TEST_OP == FP_OP_DIV
    a.FP_FIELD[0] = (fp_t)0.0;
    b.FP_FIELD[0] = (fp_t)0.0;
    a.FP_FIELD[1] = (fp_t)-3.0;
    b.FP_FIELD[1] = (fp_t)0.0;
#else
    a.FP_FIELD[0] = (fp_t)INFINITY;
    b.FP_FIELD[0] = (fp_t)INFINITY;
    a.FP_FIELD[1] = (fp_t)-0.0;
    b.FP_FIELD[1] = (fp_t)-0.0;
#endif
#if TEST_SCALAR
    __asm__ volatile("movdqu %1, %%xmm0\n\tmovdqu %2, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\t"
                     "movdqu %%xmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b)
                     : "xmm0", "xmm1", "xmm2");
#else
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " %%ymm1, %%ymm0, %%ymm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b)
                     : "ymm0", "ymm1", "ymm2");
#endif
#if TEST_OP == FP_OP_MAX || TEST_OP == FP_OP_MIN
    TEST_ASSERT(out.UINT_FIELD[0] == b.UINT_FIELD[0],
                TEST_NAME " equal signed zero comes from second source");
#if !TEST_SCALAR
    TEST_ASSERT(out.FP_FIELD[1] == b.FP_FIELD[1],
                TEST_NAME " first-source NaN selects second source");
    TEST_ASSERT(out.UINT_FIELD[2] == b.UINT_FIELD[2],
                TEST_NAME " second-source NaN payload is preserved");
#endif
#else
    TEST_ASSERT(isnan(out.FP_FIELD[0]), TEST_NAME " invalid special case is NaN");
#if TEST_OP == FP_OP_DIV && !TEST_SCALAR
    TEST_ASSERT(isinf(out.FP_FIELD[1]) && signbit(out.FP_FIELD[1]),
                TEST_NAME " signed divide by zero");
#elif !TEST_SCALAR
    TEST_ASSERT(out.FP_FIELD[1] == (fp_t)0 && signbit(out.FP_FIELD[1]),
                TEST_NAME " negative zero sign");
#endif
#endif
}

int main(void) {
    ymm_t a, b, out, initial;
    TEST_START(TEST_NAME);
    init_values(&a, &b);
#if TEST_SCALAR
    for (int i = 1; i < XMM_LANES; i++)
        a.UINT_FIELD[i] = (UINT_TYPE)(SIGN_MASK ^ (UINT_TYPE)(0x1234U + (unsigned)i));
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm2\n\tmovdqu %2, %%xmm0\n\t"
                     "movdqu %3, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(initial), "m"(a), "m"(b)
                     : "xmm0", "xmm1", "ymm2");
    TEST_ASSERT(out.FP_FIELD[0] == expected_value(a.FP_FIELD[0], b.FP_FIELD[0], 0),
                TEST_NAME " scalar result");
    for (int i = 1; i < XMM_LANES; i++)
        TEST_ASSERT(out.UINT_FIELD[i] == a.UINT_FIELD[i],
                    TEST_NAME " upper XMM lane %d comes from first source", i);
    for (int i = XMM_LANES; i < PACKED_LANES; i++)
        TEST_ASSERT(out.UINT_FIELD[i] == 0,
                    TEST_NAME " VEX scalar upper YMM lane %d", i);
    __asm__ volatile("movdqu %1, %%xmm0\n\t"
                     TEST_INSN " %2, %%xmm0, %%xmm2\n\t"
                     "movdqu %%xmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm2");
    TEST_ASSERT(out.FP_FIELD[0] == expected_value(a.FP_FIELD[0], b.FP_FIELD[0], 0),
                TEST_NAME " scalar memory source");
#else
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " %%ymm1, %%ymm0, %%ymm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b)
                     : "ymm0", "ymm1", "ymm2");
    check_packed(&out, &a, &b, PACKED_LANES, "ymm register");
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     TEST_INSN " %2, %%ymm0, %%ymm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm2");
    check_packed(&out, &a, &b, PACKED_LANES, "ymm memory");
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm2\n\tmovdqu %2, %%xmm0\n\t"
                     "movdqu %3, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(initial), "m"(a), "m"(b)
                     : "xmm0", "xmm1", "ymm2");
    check_packed(&out, &a, &b, XMM_LANES, "xmm");
    for (int i = XMM_LANES; i < PACKED_LANES; i++)
        TEST_ASSERT(out.UINT_FIELD[i] == 0,
                    TEST_NAME " VEX.128 upper lane %d", i);
#endif
    test_special_values();
    TEST_END();
}

#endif
