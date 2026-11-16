#ifndef X86_64_AVX512_VBMI2_VAR_SHIFT_TEST_H
#define X86_64_AVX512_VBMI2_VAR_SHIFT_TEST_H

#include "avx512_test.h"

static uint32_t expected_shift(uint32_t a, uint32_t b, unsigned count) {
    count &= 31U;
    if (!count) return a;
#if TEST_LEFT
    return (a << count) | (b >> (32U - count));
#else
    return (a >> count) | (b << (32U - count));
#endif
}

static void check_result(const zmm_test_t *out, const zmm_test_t *a,
                         const zmm_test_t *b, const zmm_test_t *count,
                         uint32_t mask, int zeroing) {
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U
            ? expected_shift(a->u32[i], b->u32[i], count->u32[i])
            : (zeroing ? 0 : a->u32[i]);
        TEST_ASSERT(out->u32[i] == expected,
                    TEST_NAME " lane %d: 0x%08x != 0x%08x", i,
                    out->u32[i], expected);
    }
}

int main(void) {
    static const uint32_t counts[16] = {
        0, 1, 4, 15, 31, 32, 33, 63, 2, 3, 5, 7, 16, 30, 64, 255
    };
    zmm_test_t a, b, count, out;
    uint32_t mask;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512VBMI2()) return 0;
    for (int i = 0; i < 16; i++) {
        a.u32[i] = UINT32_C(0x80000001) ^ (UINT32_C(0x10204081) * (uint32_t)i);
        b.u32[i] = UINT32_C(0x7ffffffe) ^ (UINT32_C(0x01020408) * (uint32_t)i);
    }
    memcpy(count.u32, counts, sizeof(counts));

    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t"
                     "vmovdqa32 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(count)
                     : "zmm0", "zmm1", "zmm2");
    check_result(&out, &a, &b, &count, UINT32_C(0xffff), 0);

    mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %4, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\tvmovdqa32 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(a), "m"(b), "m"(count), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    check_result(&out, &a, &b, &count, mask, 0);

    mask = UINT32_C(0x5aa5);
    __asm__ volatile("kmovw %4, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\tvmovdqa32 %3, %%zmm2\n\t"
                     TEST_INSN " %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(a), "m"(b), "m"(count), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    check_result(&out, &a, &b, &count, mask, 1);
    TEST_END();
}

#endif
