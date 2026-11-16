#ifndef X86_64_AVX_FP_UNARY_TEST_H
#define X86_64_AVX_FP_UNARY_TEST_H

#include "../common.h"
#include <math.h>

#define UNARY_SQRT 1
#define UNARY_RCP 2
#define UNARY_RSQRT 3

#if TEST_BITS == 32
typedef float unary_t;
#define UNARY_FIELD f32
#define UNARY_UINT_FIELD u32
#define UNARY_UINT uint32_t
#define UNARY_LANES 8
#define UNARY_XMM_LANES 4
#elif TEST_BITS == 64
typedef double unary_t;
#define UNARY_FIELD f64
#define UNARY_UINT_FIELD u64
#define UNARY_UINT uint64_t
#define UNARY_LANES 4
#define UNARY_XMM_LANES 2
#else
#error "TEST_BITS must be 32 or 64"
#endif

static unary_t reference(unary_t value) {
#if TEST_UNARY == UNARY_SQRT
    return (unary_t)sqrt((double)value);
#elif TEST_UNARY == UNARY_RCP
    return (unary_t)1 / value;
#else
    return (unary_t)1 / (unary_t)sqrt((double)value);
#endif
}

static int close_enough(unary_t actual, unary_t expected) {
#if TEST_UNARY == UNARY_SQRT
    return actual == expected;
#else
    unary_t error = (unary_t)fabs((double)(actual - expected));
    unary_t scale = (unary_t)fabs((double)expected);
    return error <= scale * (unary_t)0.001 + (unary_t)0.000001;
#endif
}

int main(void) {
    ymm_t src, preserved, out;
#if TEST_SCALAR
    ymm_t initial;
#endif
    TEST_START(TEST_NAME);
    memset(&src, 0, sizeof(src));
    memset(&preserved, 0, sizeof(preserved));
    for (int i = 0; i < UNARY_LANES; i++) {
        src.UNARY_FIELD[i] = (unary_t)((i + 1) * (i + 1));
        preserved.UNARY_UINT_FIELD[i] = (UNARY_UINT)(UINT64_C(0x12340000) + (unsigned)i);
    }
#if TEST_SCALAR
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm2\n\tmovdqu %2, %%xmm0\n\t"
                     "movdqu %3, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out)
                     : "m"(initial), "m"(preserved), "m"(src)
                     : "xmm0", "xmm1", "ymm2");
    TEST_ASSERT(close_enough(out.UNARY_FIELD[0], reference(src.UNARY_FIELD[0])),
                TEST_NAME " scalar result %a", (double)out.UNARY_FIELD[0]);
    for (int i = 1; i < UNARY_XMM_LANES; i++)
        TEST_ASSERT(out.UNARY_UINT_FIELD[i] == preserved.UNARY_UINT_FIELD[i],
                    TEST_NAME " preserves first-source lane %d", i);
    for (int i = UNARY_XMM_LANES; i < UNARY_LANES; i++)
        TEST_ASSERT(out.UNARY_UINT_FIELD[i] == 0,
                    TEST_NAME " VEX upper lane %d", i);
    __asm__ volatile("movdqu %1, %%xmm0\n\t"
                     TEST_INSN " %2, %%xmm0, %%xmm2\n\t"
                     "movdqu %%xmm2, %0"
                     : "=m"(out) : "m"(preserved), "m"(src)
                     : "xmm0", "xmm2");
    TEST_ASSERT(close_enough(out.UNARY_FIELD[0], reference(src.UNARY_FIELD[0])),
                TEST_NAME " scalar memory source");
#else
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     TEST_INSN " %%ymm0, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "ymm0", "ymm1");
    for (int i = 0; i < UNARY_LANES; i++)
        TEST_ASSERT(close_enough(out.UNARY_FIELD[i], reference(src.UNARY_FIELD[i])),
                    TEST_NAME " ymm lane %d", i);
    __asm__ volatile(TEST_INSN " %1, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "ymm1");
    for (int i = 0; i < UNARY_LANES; i++)
        TEST_ASSERT(close_enough(out.UNARY_FIELD[i], reference(src.UNARY_FIELD[i])),
                    TEST_NAME " memory lane %d", i);
#endif

    src.UNARY_FIELD[0] = (unary_t)-1;
    src.UNARY_FIELD[1] = (unary_t)-0.0;
#if TEST_SCALAR
    __asm__ volatile("movdqu %1, %%xmm0\n\tmovdqu %2, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2\n\t"
                     "movdqu %%xmm2, %0"
                     : "=m"(out) : "m"(preserved), "m"(src)
                     : "xmm0", "xmm1", "xmm2");
#else
    __asm__ volatile(TEST_INSN " %1, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "ymm1");
#endif
#if TEST_UNARY == UNARY_RCP
    TEST_ASSERT(out.UNARY_FIELD[0] < (unary_t)0,
                TEST_NAME " reciprocal preserves negative sign");
#else
    TEST_ASSERT(isnan(out.UNARY_FIELD[0]), TEST_NAME " negative input is NaN");
#endif
    TEST_END();
}

#endif
