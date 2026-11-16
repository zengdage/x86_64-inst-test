/* TARGET: VPCOMPRESSB */
#include "../include/avx512_test.h"

static void check_result(const zmm_test_t *out, const zmm_test_t *src,
                         uint64_t mask, int zeroing) {
    int input = 0;
    for (int i = 0; i < 64; i++) {
        while (input < 64 && !((mask >> input) & 1U)) input++;
        uint8_t expected = input < 64 ? src->u8[input++]
                                     : (zeroing ? 0 : UINT8_C(0xa5));
        TEST_ASSERT(out->u8[i] == expected,
                    "VPCOMPRESSB output byte %d", i);
    }
}

int main(void) {
    zmm_test_t src, initial, out;
    uint64_t mask;

    TEST_START("VPCOMPRESSB");
    if (!CHECK_AVX512VBMI2()) return 0;
    for (int i = 0; i < 64; i++) {
        src.u8[i] = (uint8_t)(i + 1);
        initial.u8[i] = UINT8_C(0xa5);
    }

    mask = UINT64_C(0x8000000100008429);
    __asm__ volatile("kmovq %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpcompressb %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 0);

    __asm__ volatile("kmovq %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vpcompressb %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 1);

    mask = 0;
    __asm__ volatile("kmovq %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpcompressb %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 0);
    TEST_END();
}
