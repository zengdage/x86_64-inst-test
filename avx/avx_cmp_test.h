#ifndef X86_64_AVX_CMP_TEST_H
#define X86_64_AVX_CMP_TEST_H

#include "../common.h"

#if TEST_BITS == 32
#define CMP_FIELD f32
#define CMP_UINT_FIELD u32
#define CMP_UINT uint32_t
#define CMP_ALL UINT32_MAX
#define CMP_LANES 8
#define CMP_XMM_LANES 4
#else
#define CMP_FIELD f64
#define CMP_UINT_FIELD u64
#define CMP_UINT uint64_t
#define CMP_ALL UINT64_MAX
#define CMP_LANES 4
#define CMP_XMM_LANES 2
#endif

static void init_cmp(ymm_t *a, ymm_t *b) {
    memset(a, 0, sizeof(*a));
    memset(b, 0, sizeof(*b));
    for (int i = 0; i < CMP_LANES; i++) {
        a->CMP_FIELD[i] = (__typeof__(a->CMP_FIELD[i]))(i - 2);
        b->CMP_FIELD[i] = (__typeof__(b->CMP_FIELD[i]))((i & 1) ? i - 2 : i - 1);
    }
    a->CMP_UINT_FIELD[CMP_LANES - 1] =
#if TEST_BITS == 32
        UINT32_C(0x7fc12345);
#else
        UINT64_C(0x7ff8123456789abc);
#endif
}

#if TEST_SCALAR
#define RUN_CMP(imm, a, b, out) \
    __asm__ volatile("movdqu %1, %%xmm0\n\tmovdqu %2, %%xmm1\n\t" \
                     TEST_INSN " $" #imm ", %%xmm1, %%xmm0, %%xmm2\n\t" \
                     "movdqu %%xmm2, %0" \
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2")
#else
#define RUN_CMP(imm, a, b, out) \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t" \
                     TEST_INSN " $" #imm ", %%ymm1, %%ymm0, %%ymm2\n\t" \
                     "vmovdqu %%ymm2, %0" \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2")
#endif

int main(void) {
    ymm_t a, b, out;
    TEST_START(TEST_NAME);
    init_cmp(&a, &b);
    RUN_CMP(0, a, b, out);
#if TEST_SCALAR
    TEST_ASSERT(out.CMP_UINT_FIELD[0] ==
                (a.CMP_FIELD[0] == b.CMP_FIELD[0] ? CMP_ALL : 0),
                TEST_NAME " equal predicate");
    for (int i = 1; i < CMP_XMM_LANES; i++)
        TEST_ASSERT(out.CMP_UINT_FIELD[i] == a.CMP_UINT_FIELD[i],
                    TEST_NAME " preserves first-source lane %d", i);
#else
    for (int i = 0; i < CMP_LANES; i++)
        TEST_ASSERT(out.CMP_UINT_FIELD[i] ==
                    (a.CMP_FIELD[i] == b.CMP_FIELD[i] ? CMP_ALL : 0),
                    TEST_NAME " equal lane %d", i);
#endif
    RUN_CMP(1, a, b, out);
    TEST_ASSERT(out.CMP_UINT_FIELD[0] ==
                (a.CMP_FIELD[0] < b.CMP_FIELD[0] ? CMP_ALL : 0),
                TEST_NAME " less-than predicate");
    RUN_CMP(4, a, b, out);
    TEST_ASSERT(out.CMP_UINT_FIELD[0] ==
                (a.CMP_FIELD[0] != b.CMP_FIELD[0] ? CMP_ALL : 0),
                TEST_NAME " not-equal predicate");
    a.CMP_UINT_FIELD[0] =
#if TEST_BITS == 32
        UINT32_C(0x7fc12345);
#else
        UINT64_C(0x7ff8123456789abc);
#endif
    RUN_CMP(3, a, b, out);
    TEST_ASSERT(out.CMP_UINT_FIELD[0] == CMP_ALL,
                TEST_NAME " unordered predicate");
    TEST_END();
}

#endif
