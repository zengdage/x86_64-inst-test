/* TARGET: VCVTPH2PS */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("VCVTPH2PS");
    if (!CHECK_AVX512F()) return 0;

    static const uint16_t input[16] __attribute__((aligned(32))) = {
        0x0000, 0x8000, 0x3c00, 0xc000,
        0x7bff, 0x0400, 0x0001, 0x7c00,
        0xfc00, 0x7e00, 0x3555, 0x3c01,
        0xbc00, 0x4000, 0x4200, 0x4400
    };
    static const uint32_t expected[16] = {
        UINT32_C(0x00000000), UINT32_C(0x80000000),
        UINT32_C(0x3f800000), UINT32_C(0xc0000000),
        UINT32_C(0x477fe000), UINT32_C(0x38800000),
        UINT32_C(0x33800000), UINT32_C(0x7f800000),
        UINT32_C(0xff800000), UINT32_C(0x7fc00000),
        UINT32_C(0x3eaaa000), UINT32_C(0x3f802000),
        UINT32_C(0xbf800000), UINT32_C(0x40000000),
        UINT32_C(0x40400000), UINT32_C(0x40800000)
    };
    zmm_test_t out;
    __asm__ volatile("vmovdqa %1, %%ymm0\n\t"
                     "vcvtph2ps %%ymm0, %%zmm1\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(input) : "ymm0", "zmm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == expected[i],
                    "VCVTPH2PS lane %d: %08x != %08x",
                    i, out.u32[i], expected[i]);

    uint32_t mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa %1, %%ymm0\n\t"
                     "vcvtph2ps %%ymm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(input), "r"(mask)
                     : "ymm0", "zmm1", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t value = (i == 0 || i == 15) ? expected[i] : 0;
        TEST_ASSERT(out.u32[i] == value, "VCVTPH2PS zero mask lane %d", i);
    }
    TEST_END();
}
