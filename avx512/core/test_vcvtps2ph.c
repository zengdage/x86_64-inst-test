/* TARGET: VCVTPS2PH */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("VCVTPS2PH");
    if (!CHECK_AVX512F()) return 0;

    zmm_test_t input;
    static const uint32_t bits[16] = {
        UINT32_C(0x00000000), UINT32_C(0x80000000),
        UINT32_C(0x3f800000), UINT32_C(0xc0000000),
        UINT32_C(0x477fe000), UINT32_C(0x38800000),
        UINT32_C(0x33800000), UINT32_C(0x7f800000),
        UINT32_C(0xff800000), UINT32_C(0x7fc00000),
        UINT32_C(0x3eaaa000), UINT32_C(0x3f802000),
        UINT32_C(0xbf800000), UINT32_C(0x40000000),
        UINT32_C(0x40400000), UINT32_C(0x40800000)
    };
    static const uint16_t expected[16] = {
        0x0000, 0x8000, 0x3c00, 0xc000,
        0x7bff, 0x0400, 0x0001, 0x7c00,
        0xfc00, 0x7e00, 0x3555, 0x3c01,
        0xbc00, 0x4000, 0x4200, 0x4400
    };
    for (int i = 0; i < 16; i++) input.u32[i] = bits[i];

    uint16_t out[16] __attribute__((aligned(32)));
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\t"
                     "vcvtps2ph $0, %%zmm0, %%ymm1\n\t"
                     "vmovdqa %%ymm1, %0"
                     : "=m"(out) : "m"(input) : "zmm0", "ymm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out[i] == expected[i],
                    "VCVTPS2PH lane %d: %04x != %04x", i, out[i], expected[i]);

    uint32_t mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vcvtps2ph $0, %%zmm0, %%ymm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa %%ymm1, %0"
                     : "=m"(out) : "m"(input), "r"(mask)
                     : "zmm0", "ymm1", "k1");
    for (int i = 0; i < 16; i++) {
        uint16_t value = (i == 0 || i == 15) ? expected[i] : 0;
        TEST_ASSERT(out[i] == value, "VCVTPS2PH zero mask lane %d", i);
    }
    TEST_END();
}
