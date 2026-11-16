/* TARGET: VPCOMPRESSQ */
#include "../include/avx512_test.h"

static void check_result(const zmm_test_t *out, const zmm_test_t *src,
                         uint32_t mask, int zeroing) {
    int source = 0;
    for (int lane = 0; lane < 8; lane++) {
        while (source < 8 && !((mask >> source) & 1U)) source++;
        uint64_t expected = source < 8 ? src->u64[source++]
                                      : (zeroing ? 0 : UINT64_C(0xdeadbeefcafebabe));
        TEST_ASSERT(out->u64[lane] == expected,
                    "VPCOMPRESSQ lane %d", lane);
    }
}

int main(void) {
    zmm_test_t src, initial, out;
    uint32_t mask;

    TEST_START("VPCOMPRESSQ");
    if (!CHECK_AVX512F()) return 0;
    for (int i = 0; i < 8; i++) {
        src.u64[i] = UINT64_C(0x100000000) + (uint64_t)i;
        initial.u64[i] = UINT64_C(0xdeadbeefcafebabe);
    }

    mask = UINT32_C(0x81);
    __asm__ volatile("kmovb %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpcompressq %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 0);

    mask = UINT32_C(0xa5);
    __asm__ volatile("kmovb %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vpcompressq %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, &src, mask, 1);
    TEST_END();
}
