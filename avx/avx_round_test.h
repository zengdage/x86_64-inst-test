#ifndef X86_64_AVX_ROUND_TEST_H
#define X86_64_AVX_ROUND_TEST_H

#include "../common.h"
#include <math.h>

#if TEST_BITS == 32
#define TEST_FIELD f32
#define TEST_UINT_FIELD u32
#define TEST_LANES 8
#define TEST_XMM_LANES 4
static const ymm_t input = { .f32 = {
    2.5f, 3.5f, -2.5f, -3.5f, 2.3f, -2.3f, INFINITY, -0.0f
} };
static const ymm_t nearest = { .f32 = {
    2.0f, 4.0f, -2.0f, -4.0f, 2.0f, -2.0f, INFINITY, -0.0f
} };
static const ymm_t down = { .f32 = {
    2.0f, 3.0f, -3.0f, -4.0f, 2.0f, -3.0f, INFINITY, -0.0f
} };
static const ymm_t up = { .f32 = {
    3.0f, 4.0f, -2.0f, -3.0f, 3.0f, -2.0f, INFINITY, -0.0f
} };
static const ymm_t truncation = { .f32 = {
    2.0f, 3.0f, -2.0f, -3.0f, 2.0f, -2.0f, INFINITY, -0.0f
} };
#elif TEST_BITS == 64
#define TEST_FIELD f64
#define TEST_UINT_FIELD u64
#define TEST_LANES 4
#define TEST_XMM_LANES 2
static const ymm_t input = { .f64 = {2.5, 3.5, -2.3, -0.0} };
static const ymm_t nearest = { .f64 = {2.0, 4.0, -2.0, -0.0} };
static const ymm_t down = { .f64 = {2.0, 3.0, -3.0, -0.0} };
static const ymm_t up = { .f64 = {3.0, 4.0, -2.0, -0.0} };
static const ymm_t truncation = { .f64 = {2.0, 3.0, -2.0, -0.0} };
#else
#error "TEST_BITS must be 32 or 64"
#endif

static void check(const ymm_t *out, const ymm_t *expected, int lanes,
                  const char *mode) {
    for (int i = 0; i < lanes; i++)
        TEST_ASSERT(out->TEST_UINT_FIELD[i] == expected->TEST_UINT_FIELD[i],
                    TEST_NAME " %s lane %d: got %a expected %a", mode, i,
                    (double)out->TEST_FIELD[i], (double)expected->TEST_FIELD[i]);
}

#define RUN_ROUND(imm, src, out) \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" \
                     TEST_INSN " $" #imm ", %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0" \
                     : "=m"(out) : "m"(src) : "ymm0", "ymm1")

int main(void) {
    ymm_t out, initial;
    TEST_START(TEST_NAME);
    RUN_ROUND(0, input, out);
    check(&out, &nearest, TEST_LANES, "nearest");
    RUN_ROUND(1, input, out);
    check(&out, &down, TEST_LANES, "down");
    RUN_ROUND(2, input, out);
    check(&out, &up, TEST_LANES, "up");
    RUN_ROUND(3, input, out);
    check(&out, &truncation, TEST_LANES, "truncate");

    __asm__ volatile(TEST_INSN " $0xf3, %1, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(input) : "ymm1");
    check(&out, &truncation, TEST_LANES, "ignored high immediate bits");

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\tmovdqu %2, %%xmm0\n\t"
                     TEST_INSN " $0, %%xmm0, %%xmm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(input)
                     : "xmm0", "ymm1");
    check(&out, &nearest, TEST_XMM_LANES, "xmm");
    for (int i = TEST_XMM_LANES; i < TEST_LANES; i++)
        TEST_ASSERT(out.TEST_UINT_FIELD[i] == 0,
                    TEST_NAME " VEX.128 upper lane %d", i);
    TEST_END();
}

#endif
