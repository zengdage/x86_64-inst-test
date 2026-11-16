/* TARGET: VCOMPRESSPS */
#include "../include/avx512_test.h"

static void check_result(const zmm_test_t *out, const zmm_test_t *src,
                         uint32_t mask, int zeroing) {
    int source = 0;
    for (int lane = 0; lane < 16; lane++) {
        while (source < 16 && !((mask >> source) & 1U)) source++;
        uint32_t expected = source < 16 ? src->u32[source++]
                                       : (zeroing ? 0 : UINT32_C(0xdeadbeef));
        TEST_ASSERT(out->u32[lane] == expected,
                    "VCOMPRESSPS bit-exact lane %d", lane);
    }
}

int main(void) {
    zmm_test_t src, initial, out;
    uint32_t mask;

    TEST_START("VCOMPRESSPS");
    if (!CHECK_AVX512F()) return 0;
    for (int i = 0; i < 16; i++) {
        src.u32[i] = UINT32_C(0x7fc00000) + (uint32_t)i;
        initial.u32[i] = UINT32_C(0xdeadbeef);
    }

    mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\t"
                     "vcompressps %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 0);

    mask = UINT32_C(0xa55a);
    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vcompressps %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 1);
    TEST_END();
}
