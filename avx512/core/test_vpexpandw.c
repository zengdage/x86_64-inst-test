/* TARGET: VPEXPANDW */
#include "../include/avx512_test.h"

static void check_result(const zmm_test_t *out, const zmm_test_t *src,
                         uint32_t mask, int zeroing) {
    int input = 0;
    for (int i = 0; i < 32; i++) {
        uint16_t expected = (mask >> i) & 1U
            ? src->u16[input++] : (zeroing ? 0 : UINT16_C(0xa5a5));
        TEST_ASSERT(out->u16[i] == expected, "VPEXPANDW lane %d", i);
    }
}

int main(void) {
    zmm_test_t src, initial, out;
    uint32_t mask;

    TEST_START("VPEXPANDW");
    if (!CHECK_AVX512VBMI2()) return 0;
    for (int i = 0; i < 32; i++) {
        src.u16[i] = (uint16_t)(0x1000 + i);
        initial.u16[i] = UINT16_C(0xa5a5);
    }

    mask = UINT32_C(0x80018429);
    __asm__ volatile("kmovd %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpexpandw %%zmm0, %%zmm1%{%%k1%}\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 0);

    __asm__ volatile("kmovd %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vpexpandw %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 1);
    TEST_END();
}
