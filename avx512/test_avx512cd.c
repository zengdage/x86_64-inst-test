/* AVX-512CD conflict detection and vector leading-zero count. */
#include "avx512_test.h"

static void test_conflict_dword(void) {
    zmm_test_t src, out;
    const uint32_t values[16] = {7, 1, 7, 2, 1, 7, 3, 3, 0, 2, 0, 7, 4, 4, 4, 1};
    memcpy(src.u32, values, sizeof(values));
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvpconflictd %%zmm0, %%zmm1\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = 0;
        for (int j = 0; j < i; j++)
            if (src.u32[j] == src.u32[i]) expected |= UINT32_C(1) << j;
        TEST_ASSERT(out.u32[i] == expected, "VPCONFLICTD lane %d: 0x%x", i, out.u32[i]);
    }

    uint32_t mask = 0x8001U;
    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vpconflictd %%zmm0, %%zmm0%{%%k1%}%{z%}\n\tvmovdqa32 %%zmm0, %0"
                     : "=m"(out) : "m"(src), "r"(mask) : "zmm0", "k1");
    TEST_ASSERT(out.u32[0] == 0 && out.u32[15] == 0x12U,
                "VPCONFLICTD masked overlap first/high lanes");
    for (int i = 1; i < 15; i++) TEST_ASSERT(out.u32[i] == 0, "VPCONFLICTD zero mask lane %d", i);
}

static void test_conflict_qword(void) {
    zmm_test_t src = {.u64 = {9, 2, 9, 9, 2, 5, 5, 9}}, out;
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvpconflictq %%zmm0, %%zmm0\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(src) : "zmm0");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = 0;
        for (int j = 0; j < i; j++)
            if (src.u64[j] == src.u64[i]) expected |= UINT64_C(1) << j;
        TEST_ASSERT(out.u64[i] == expected, "VPCONFLICTQ lane %d", i);
    }
}

static void test_lzcnt(void) {
    zmm_test_t src, dword, qword;
    const uint32_t dvalues[16] = {0, 1, 2, 3, 0x80000000U, 0xffffffffU, 0x10000U, 0xffffU,
                                  4, 8, 16, 32, 64, 128, 256, 512};
    memcpy(src.u32, dvalues, sizeof(dvalues));
    __asm__ volatile("vmovdqa32 %2, %%zmm0\n\tvplzcntd %%zmm0, %%zmm1\n\t"
                     "vplzcntq %%zmm0, %%zmm2\n\tvmovdqa32 %%zmm1, %0\n\tvmovdqa64 %%zmm2, %1"
                     : "=m"(dword), "=m"(qword) : "m"(src) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = src.u32[i] ? (uint32_t)__builtin_clz(src.u32[i]) : 32U;
        TEST_ASSERT(dword.u32[i] == expected, "VPLZCNTD lane %d", i);
    }
    for (int i = 0; i < 8; i++) {
        uint64_t expected = src.u64[i] ? (uint64_t)__builtin_clzll(src.u64[i]) : 64U;
        TEST_ASSERT(qword.u64[i] == expected, "VPLZCNTQ lane %d", i);
    }
}

int main(void) {
    TEST_START("AVX-512CD VPCONFLICT/VPLZCNT");
    if (!CHECK_AVX512CD()) return 0;
    test_conflict_dword();
    test_conflict_qword();
    test_lzcnt();
    TEST_END();
}
