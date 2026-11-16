/* TARGET: VPCONFLICTQ */
#include "../include/avx512_test.h"

static uint64_t expected_conflict(const zmm_test_t *src, int lane) {
    uint64_t expected = 0;
    for (int i = 0; i < lane; i++)
        if (src->u64[i] == src->u64[lane])
            expected |= UINT64_C(1) << i;
    return expected;
}

int main(void) {
    zmm_test_t src = {.u64 = {9, 2, 9, 9, 2, 5, 5, 9}};
    zmm_test_t initial, out;
    uint32_t mask;

    TEST_START("VPCONFLICTQ");
    if (!CHECK_AVX512CD()) return 0;
    for (int i = 0; i < 8; i++)
        initial.u64[i] = UINT64_C(0xdeadbeefcafebabe);

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\t"
                     "vpconflictq %%zmm0, %%zmm1\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == expected_conflict(&src, i),
                    "VPCONFLICTQ lane %d", i);

    mask = UINT32_C(0x81);
    __asm__ volatile("kmovb %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpconflictq %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (mask >> i) & 1U
            ? expected_conflict(&src, i) : UINT64_C(0xdeadbeefcafebabe);
        TEST_ASSERT(out.u64[i] == expected,
                    "VPCONFLICTQ merge mask lane %d", i);
    }

    mask = UINT32_C(0x5a);
    __asm__ volatile("kmovb %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vpconflictq %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (mask >> i) & 1U ? expected_conflict(&src, i) : 0;
        TEST_ASSERT(out.u64[i] == expected,
                    "VPCONFLICTQ zero mask lane %d", i);
    }
    TEST_END();
}
