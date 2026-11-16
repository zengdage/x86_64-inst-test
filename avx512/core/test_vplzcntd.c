/* TARGET: VPLZCNTD */
#include "../include/avx512_test.h"

static uint32_t expected_lzcnt(uint32_t value) {
    return value ? (uint32_t)__builtin_clz(value) : 32U;
}

int main(void) {
    static const uint32_t values[16] = {
        0, 1, 2, 3, UINT32_C(0x80000000), UINT32_MAX, UINT32_C(0x10000),
        UINT32_C(0xffff), 4, 8, 16, 32, 64, 128, 256, 512
    };
    zmm_test_t src, initial, out;
    uint32_t mask;

    TEST_START("VPLZCNTD");
    if (!CHECK_AVX512CD()) return 0;
    memcpy(src.u32, values, sizeof(values));
    for (int i = 0; i < 16; i++) initial.u32[i] = UINT32_C(0xdeadbeef);

    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\t"
                     "vplzcntd %%zmm0, %%zmm1\n\tvmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == expected_lzcnt(src.u32[i]),
                    "VPLZCNTD lane %d", i);

    mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\t"
                     "vplzcntd %%zmm0, %%zmm1%{%%k1%}\n\tvmovdqa32 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U
            ? expected_lzcnt(src.u32[i]) : UINT32_C(0xdeadbeef);
        TEST_ASSERT(out.u32[i] == expected, "VPLZCNTD merge mask lane %d", i);
    }

    mask = UINT32_C(0x5aa5);
    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vplzcntd %%zmm0, %%zmm1%{%%k1%}%{z%}\n\tvmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U ? expected_lzcnt(src.u32[i]) : 0;
        TEST_ASSERT(out.u32[i] == expected, "VPLZCNTD zero mask lane %d", i);
    }
    TEST_END();
}
