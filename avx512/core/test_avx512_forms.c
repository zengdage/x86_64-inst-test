/* Remaining AVX-512F/BW/DQ/VL instruction forms supported by GAS 2.38. */
#include "../include/avx512_test.h"
#include <math.h>

static void test_align_shuffle(void) {
    zmm_test_t a, b, out;
    for (int i = 0; i < 16; i++) { a.u32[i] = 0x1000U + (uint32_t)i; b.u32[i] = 0x2000U + (uint32_t)i; }
    zmm_test_t shuffled;
    __asm__ volatile("vmovdqa32 %2, %%zmm0\n\tvmovdqa32 %3, %%zmm1\n\t"
                     "valignd $4, %%zmm1, %%zmm0, %%zmm2\n\t"
                     "vshuff32x4 $0x1b, %%zmm1, %%zmm0, %%zmm3\n\t"
                     "vmovdqa32 %%zmm2, %0\n\tvmovdqa32 %%zmm3, %1"
                     : "=m"(out), "=m"(shuffled) : "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2", "zmm3");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = i < 12 ? b.u32[i + 4] : a.u32[i - 12];
        TEST_ASSERT(out.u32[i] == expected, "VALIGND lane %d", i);
    }
    TEST_ASSERT(shuffled.u32[0] == a.u32[12], "VSHUFF32X4 output capture");
}

static void test_permutations(void) {
    zmm_test_t index, src1, src2, out;
    for (int i = 0; i < 16; i++) { index.u32[i] = (uint32_t)(i * 3) & 31U; src1.u32[i] = 0x1000U + (uint32_t)i; src2.u32[i] = 0x2000U + (uint32_t)i; }
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\tvmovdqa32 %3, %%zmm2\n\t"
                     "vpermt2d %%zmm1, %%zmm0, %%zmm2\n\tvmovdqa32 %%zmm2, %0"
                     : "=m"(out) : "m"(index), "m"(src1), "m"(src2)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) {
        uint32_t idx = index.u32[i];
        uint32_t expected = idx < 16 ? src2.u32[idx] : src1.u32[idx - 16];
        TEST_ASSERT(out.u32[i] == expected, "VPERMT2D lane %d", i);
    }
    for (int i = 0; i < 8; i++) { index.u64[i] = (uint64_t)(i * 5) & 15U; src1.u64[i] = 0x100000000ULL + (uint64_t)i; src2.u64[i] = 0x200000000ULL + (uint64_t)i; }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
                     "vpermt2q %%zmm1, %%zmm0, %%zmm2\n\tvmovdqa64 %%zmm2, %0"
                     : "=m"(out) : "m"(index), "m"(src1), "m"(src2)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) TEST_ASSERT(out.u64[i] == (index.u64[i] < 8 ? src2.u64[index.u64[i]] : src1.u64[index.u64[i] - 8]), "VPERMT2Q lane %d", i);
}

static void test_compress_extract_insert(void) {
    zmm_test_t src, out;
    for (int i = 0; i < 8; i++) src.u64[i] = UINT64_C(0x100000000) + (uint64_t)i;
    uint32_t mask = 0xa5U;
    __asm__ volatile("kmovb %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\tvcompresspd %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask) : "zmm0", "zmm1", "k1");
    int packed = 0;
    for (int i = 0; i < 8; i++) { if ((mask >> i) & 1U) TEST_ASSERT(out.u64[packed++] == src.u64[i], "VCOMPRESSPD packed %d", packed - 1); }
    for (; packed < 8; packed++) TEST_ASSERT(out.u64[packed] == 0, "VCOMPRESSPD zero tail %d", packed);

    ymm_t extracted;
    zmm_test_t wide;
    for (int i = 0; i < 16; i++) wide.u32[i] = 0xabc00000U + (uint32_t)i;
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvextractf32x8 $1, %%zmm0, %0"
                     : "=m"(extracted) : "m"(wide) : "zmm0");
    TEST_ASSERT(extracted.u32[0] == wide.u32[8], "VEXTRACTF32X8 upper source");
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvextractf64x4 $1, %%zmm0, %0"
                     : "=m"(out) : "m"(src) : "zmm0");
    TEST_ASSERT(out.u64[0] == src.u64[4], "VEXTRACTF64X4 upper half");
}

static void test_mask_tests_and_narrow(void) {
    zmm_test_t src, out;
    for (int i = 0; i < 16; i++) src.u32[i] = (i & 1) ? 0x80000000U : 1U;
    uint64_t mask;
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvptestmb %%zmm0, %%zmm0, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(mask) : "m"(src) : "zmm0", "k1");
    uint64_t expected_mask = 0;
    for (int i = 0; i < 64; i++) expected_mask |= (uint64_t)(src.u8[i] != 0) << i;
    TEST_ASSERT(mask == expected_mask, "VPTESTMB nonzero mask");
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvpmovdb %%zmm0, %%xmm1\n\tvmovdqu %%xmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "xmm1");
    for (int i = 0; i < 4; i++) TEST_ASSERT(out.u8[i] == (uint8_t)src.u32[i], "VPMOVDB lane %d", i);
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvpmovdw %%zmm0, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "ymm1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(out.u16[i] == (uint16_t)src.u32[i], "VPMOVDW lane %d", i);
}

static void test_fixup_and_sad(void) {
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 16; i++) { a.f32[i] = i == 0 ? NAN : (float)i; b.f32[i] = 1.0f; c.f32[i] = 2.0f; }
    __asm__ volatile("vmovaps %1, %%zmm0\n\tvmovaps %2, %%zmm1\n\tvmovaps %3, %%zmm2\n\t"
                     "vfixupimmps $0x00, %%zmm2, %%zmm1, %%zmm0\n\tvmovaps %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(IS_QNAN(out.f32[0]) && out.f32[1] == 1.0f, "VFIXUPIMMPS NaN/default fixup");
    for (int i = 0; i < 64; i++) { a.u8[i] = (uint8_t)i; b.u8[i] = (uint8_t)(63 - i); }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\tvdbpsadbw $0x00, %%zmm1, %%zmm0, %%zmm2\n\tvmovdqa64 %%zmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(out.u16[0] == 0xf0 && out.u16[1] == 0xf0 && out.u16[2] == 0xe0,
                "VDBPSADBW first SAD groups");
}

int main(void) {
    TEST_START("Remaining AVX-512 foundation instruction forms");
    if (!CHECK_AVX512F()) return 0;
    test_align_shuffle();
    test_permutations();
    test_compress_extract_insert();
    test_mask_tests_and_narrow();
    test_fixup_and_sad();
    TEST_END();
}
