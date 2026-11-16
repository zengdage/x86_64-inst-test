#ifndef X86_64_AVX512_BLENDM_TEST_H
#define X86_64_AVX512_BLENDM_TEST_H

#include "avx512_test.h"

#if TEST_BITS == 32
#define TEST_FIELD u32
#define TEST_XMM_LANES 4
#define TEST_YMM_LANES 8
#define TEST_ZMM_LANES 16
#define TEST_MOVE "vmovdqa32"
#define TEST_ALL_MASK UINT32_C(0xffff)
#define TEST_ENDPOINT_MASK UINT32_C(0x8001)
#define TEST_MIXED_MASK UINT32_C(0xa55a)
typedef uint32_t test_uint_t;
#elif TEST_BITS == 64
#define TEST_FIELD u64
#define TEST_XMM_LANES 2
#define TEST_YMM_LANES 4
#define TEST_ZMM_LANES 8
#define TEST_MOVE "vmovdqa64"
#define TEST_ALL_MASK UINT32_C(0xff)
#define TEST_ENDPOINT_MASK UINT32_C(0x81)
#define TEST_MIXED_MASK UINT32_C(0x5a)
typedef uint64_t test_uint_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

static void check_lanes(const zmm_test_t *out, const zmm_test_t *a,
                        const zmm_test_t *b, int lanes, uint32_t mask,
                        int zeroing) {
    for (int i = 0; i < lanes; i++) {
        test_uint_t expected = (mask >> i) & 1U
            ? b->TEST_FIELD[i] : (zeroing ? 0 : a->TEST_FIELD[i]);
        TEST_ASSERT(out->TEST_FIELD[i] == expected,
                    TEST_NAME " %d-bit lane %d", lanes * TEST_BITS, i);
    }
}

int main(void) {
    zmm_test_t a, b, out;
    uint32_t mask;

    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F() || !CHECK_AVX512VL()) return 0;
    for (int i = 0; i < TEST_ZMM_LANES; i++) {
        a.TEST_FIELD[i] = (test_uint_t)(UINT64_C(0x1020304050607080) + (uint64_t)i);
        b.TEST_FIELD[i] = (test_uint_t)(UINT64_C(0x90a0b0c0d0e0f000) + (uint64_t)i);
    }

    mask = (UINT32_C(1) << 0) | (UINT32_C(1) << (TEST_XMM_LANES - 1));
    __asm__ volatile("kmovw %3, %%k1\n\t" TEST_MOVE " %1, %%xmm0\n\t"
                     TEST_MOVE " %2, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0, %%xmm2%{%%k1%}\n\t"
                     TEST_MOVE " %%xmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b), "r"(mask)
                     : "xmm0", "xmm1", "xmm2", "k1");
    check_lanes(&out, &a, &b, TEST_XMM_LANES, mask, 0);

    mask = (UINT32_C(1) << 1) | (UINT32_C(1) << (TEST_YMM_LANES - 1));
    __asm__ volatile("kmovw %3, %%k1\n\t" TEST_MOVE " %1, %%ymm0\n\t"
                     TEST_MOVE " %2, %%ymm1\n\t"
                     TEST_INSN " %%ymm1, %%ymm0, %%ymm2%{%%k1%}%{z%}\n\t"
                     TEST_MOVE " %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b), "r"(mask)
                     : "ymm0", "ymm1", "ymm2", "k1");
    check_lanes(&out, &a, &b, TEST_YMM_LANES, mask, 1);

    static const uint32_t masks[] = {
        0, TEST_ALL_MASK, 1, TEST_ENDPOINT_MASK, TEST_MIXED_MASK
    };
    for (unsigned m = 0; m < sizeof(masks) / sizeof(masks[0]); m++) {
        mask = masks[m];
        __asm__ volatile("kmovw %3, %%k1\n\t" TEST_MOVE " %1, %%zmm0\n\t"
                         TEST_MOVE " %2, %%zmm1\n\t"
                         TEST_INSN " %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
                         TEST_MOVE " %%zmm2, %0"
                         : "=m"(out) : "m"(a), "m"(b), "r"(mask)
                         : "zmm0", "zmm1", "zmm2", "k1");
        check_lanes(&out, &a, &b, TEST_ZMM_LANES, mask, 0);

        __asm__ volatile("kmovw %3, %%k1\n\t" TEST_MOVE " %1, %%zmm0\n\t"
                         TEST_MOVE " %2, %%zmm1\n\t"
                         TEST_INSN " %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
                         TEST_MOVE " %%zmm2, %0"
                         : "=m"(out) : "m"(a), "m"(b), "r"(mask)
                         : "zmm0", "zmm1", "zmm2", "k1");
        check_lanes(&out, &a, &b, TEST_ZMM_LANES, mask, 1);
    }
    TEST_END();
}

#endif
