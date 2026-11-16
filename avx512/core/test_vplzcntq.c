/* TARGET: VPLZCNTQ */
#include "../include/avx512_test.h"

static uint64_t expected_lzcnt(uint64_t value) {
    return value ? (uint64_t)__builtin_clzll(value) : 64U;
}

int main(void) {
    zmm_test_t src = {.u64 = {
        0, 1, 2, 3, UINT64_C(0x8000000000000000), UINT64_MAX,
        UINT64_C(0x100000000), UINT64_C(0xffffffff)
    }};
    zmm_test_t initial, out;
    uint32_t mask;

    TEST_START("VPLZCNTQ");
    if (!CHECK_AVX512CD()) return 0;
    for (int i = 0; i < 8; i++)
        initial.u64[i] = UINT64_C(0xdeadbeefcafebabe);

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\t"
                     "vplzcntq %%zmm0, %%zmm1\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == expected_lzcnt(src.u64[i]),
                    "VPLZCNTQ lane %d", i);

    mask = UINT32_C(0x81);
    __asm__ volatile("kmovb %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vplzcntq %%zmm0, %%zmm1%{%%k1%}\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (mask >> i) & 1U
            ? expected_lzcnt(src.u64[i]) : UINT64_C(0xdeadbeefcafebabe);
        TEST_ASSERT(out.u64[i] == expected, "VPLZCNTQ merge mask lane %d", i);
    }

    mask = UINT32_C(0x5a);
    __asm__ volatile("kmovb %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vplzcntq %%zmm0, %%zmm1%{%%k1%}%{z%}\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (mask >> i) & 1U ? expected_lzcnt(src.u64[i]) : 0;
        TEST_ASSERT(out.u64[i] == expected, "VPLZCNTQ zero mask lane %d", i);
    }
    TEST_END();
}
