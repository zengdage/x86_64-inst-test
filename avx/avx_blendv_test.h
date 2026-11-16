#ifndef X86_64_AVX_BLENDV_TEST_H
#define X86_64_AVX_BLENDV_TEST_H

#include "../common.h"

#if TEST_BITS == 32
#define TEST_FIELD u32
#define TEST_LANES 8
#define TEST_XMM_LANES 4
#define TEST_SIGN UINT32_C(0x80000000)
#elif TEST_BITS == 64
#define TEST_FIELD u64
#define TEST_LANES 4
#define TEST_XMM_LANES 2
#define TEST_SIGN UINT64_C(0x8000000000000000)
#else
#error "TEST_BITS must be 32 or 64"
#endif

static void check_result(const ymm_t *out, const ymm_t *a, const ymm_t *b,
                         const ymm_t *mask, int lanes) {
    for (int i = 0; i < lanes; i++) {
        __typeof__(a->TEST_FIELD[i]) expected =
            mask->TEST_FIELD[i] & TEST_SIGN ? b->TEST_FIELD[i] : a->TEST_FIELD[i];
        TEST_ASSERT(out->TEST_FIELD[i] == expected,
                    TEST_NAME " lane %d mask sign", i);
    }
}

int main(void) {
    ymm_t a, b, mask, out, initial;
    TEST_START(TEST_NAME);
    for (int i = 0; i < TEST_LANES; i++) {
        a.TEST_FIELD[i] = (__typeof__(a.TEST_FIELD[i]))(UINT64_C(0x11110000) + (unsigned)i);
        b.TEST_FIELD[i] = (__typeof__(b.TEST_FIELD[i]))(UINT64_C(0xeeee0000) + (unsigned)i);
        mask.TEST_FIELD[i] = (i == 0 || i == TEST_LANES - 1 || (i & 2)) ?
            TEST_SIGN : (TEST_SIGN - 1);
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vmovdqu %3, %%ymm2\n\t"
                     TEST_INSN " %%ymm2, %%ymm1, %%ymm0, %%ymm3\n\t"
                     "vmovdqu %%ymm3, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(mask)
                     : "ymm0", "ymm1", "ymm2", "ymm3");
    check_result(&out, &a, &b, &mask, TEST_LANES);

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm2\n\t"
                     TEST_INSN " %%ymm2, %3, %%ymm0, %%ymm3\n\t"
                     "vmovdqu %%ymm3, %0"
                     : "=m"(out) : "m"(a), "m"(mask), "m"(b)
                     : "ymm0", "ymm2", "ymm3");
    check_result(&out, &a, &b, &mask, TEST_LANES);

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm3\n\tmovdqu %2, %%xmm0\n\t"
                     "movdqu %3, %%xmm1\n\tmovdqu %4, %%xmm2\n\t"
                     TEST_INSN " %%xmm2, %%xmm1, %%xmm0, %%xmm3\n\t"
                     "vmovdqu %%ymm3, %0"
                     : "=m"(out)
                     : "m"(initial), "m"(a), "m"(b), "m"(mask)
                     : "xmm0", "xmm1", "xmm2", "ymm3");
    check_result(&out, &a, &b, &mask, TEST_XMM_LANES);
    for (int i = TEST_XMM_LANES; i < TEST_LANES; i++)
        TEST_ASSERT(out.TEST_FIELD[i] == 0,
                    TEST_NAME " VEX.128 upper lane %d", i);
    TEST_END();
}

#endif
