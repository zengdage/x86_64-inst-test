/* AVX-512F qword register expand. */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("AVX-512F VPEXPANDQ");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t src, initial, merged, zeroed;
    for (int i = 0; i < 8; i++) {
        src.u64[i] = UINT64_C(0x1111000000000000) + (uint64_t)i;
        initial.u64[i] = UINT64_C(0xdead000000000000) + (uint64_t)i;
    }
    const uint32_t mask = 0xa5U;
    __asm__ volatile("kmovb %3, %%k1\n\tvmovdqa64 %4, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\tvpexpandq %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0\n\tvpexpandq %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm2, %1"
                     : "=m"(merged), "=m"(zeroed)
                     : "m"(initial), "r"(mask), "m"(src)
                     : "zmm0", "zmm1", "zmm2", "k1");
    int consumed = 0;
    for (int i = 0; i < 8; i++) {
        uint64_t active = ((mask >> i) & 1U) ? src.u64[consumed++] : 0;
        TEST_ASSERT(merged.u64[i] == (((mask >> i) & 1U) ? active : initial.u64[i]),
                    "VPEXPANDQ merge lane %d", i);
        TEST_ASSERT(zeroed.u64[i] == active, "VPEXPANDQ zero lane %d", i);
    }
    TEST_END();
}
