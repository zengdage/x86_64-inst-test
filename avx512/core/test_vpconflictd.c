/* TARGET: VPCONFLICTD */
#include "../include/avx512_test.h"

static uint32_t expected_conflict(const zmm_test_t *src, int lane) {
    uint32_t expected = 0;
    for (int i = 0; i < lane; i++)
        if (src->u32[i] == src->u32[lane])
            expected |= UINT32_C(1) << i;
    return expected;
}

int main(void) {
    static const uint32_t values[16] = {
        7, 1, 7, 2, 1, 7, 3, 3, 0, 2, 0, 7, 4, 4, 4, 1
    };
    zmm_test_t src, initial, out;
    uint32_t mask;

    TEST_START("VPCONFLICTD");
    if (!CHECK_AVX512CD()) return 0;
    memcpy(src.u32, values, sizeof(values));
    for (int i = 0; i < 16; i++) initial.u32[i] = UINT32_C(0xdeadbeef);

    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\t"
                     "vpconflictd %%zmm0, %%zmm1\n\tvmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == expected_conflict(&src, i),
                    "VPCONFLICTD lane %d", i);

    mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\t"
                     "vpconflictd %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U
            ? expected_conflict(&src, i) : UINT32_C(0xdeadbeef);
        TEST_ASSERT(out.u32[i] == expected,
                    "VPCONFLICTD merge mask lane %d", i);
    }

    mask = UINT32_C(0x5aa5);
    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vpconflictd %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U ? expected_conflict(&src, i) : 0;
        TEST_ASSERT(out.u32[i] == expected,
                    "VPCONFLICTD zero mask lane %d", i);
    }
    TEST_END();
}
