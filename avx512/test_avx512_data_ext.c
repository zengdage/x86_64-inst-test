/* Additional AVX-512 broadcast, insert, and two-table permutation forms. */
#include "avx512_test.h"

static void assert_repeated_u32(const zmm_test_t *out, const uint32_t *src,
                                int block, const char *name) {
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out->u32[i] == src[i % block], "%s lane %d", name, i);
}

static void assert_repeated_u64(const zmm_test_t *out, const uint64_t *src,
                                int block, const char *name) {
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out->u64[i] == src[i % block], "%s lane %d", name, i);
}

static void test_broadcasts(void) {
    zmm_test_t src, out;
    for (int i = 0; i < 16; i++) src.u32[i] = UINT32_C(0x10203040) + (uint32_t)i;

#define TEST_BROADCAST(INST, BLOCK, LABEL) do { \
    __asm__ volatile(#INST " %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0" \
                     : "=m"(out) : "m"(src) : "zmm0"); \
    assert_repeated_u32(&out, src.u32, BLOCK, LABEL); \
} while (0)
    TEST_BROADCAST(vbroadcastf32x2, 2, "VBROADCASTF32X2");
    TEST_BROADCAST(vbroadcastf32x8, 8, "VBROADCASTF32X8");
    TEST_BROADCAST(vbroadcasti32x2, 2, "VBROADCASTI32X2");
    TEST_BROADCAST(vbroadcasti32x8, 8, "VBROADCASTI32X8");
#undef TEST_BROADCAST

    __asm__ volatile("vbroadcasti64x2 %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(src) : "zmm0");
    assert_repeated_u64(&out, src.u64, 2, "VBROADCASTI64X2");
    __asm__ volatile("vbroadcasti64x4 %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(src) : "zmm0");
    assert_repeated_u64(&out, src.u64, 4, "VBROADCASTI64X4");

    uint32_t mask = UINT32_C(0xa5c3);
    __asm__ volatile("kmovw %1, %%k1\n\tvpbroadcastmb2q %%k1, %%zmm0\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "r"(mask) : "k1", "zmm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (mask & UINT32_C(0xff)),
                    "VPBROADCASTMB2Q lane %d", i);
    __asm__ volatile("kmovw %1, %%k1\n\tvpbroadcastmw2d %%k1, %%zmm0\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "r"(mask) : "k1", "zmm0");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == (mask & UINT32_C(0xffff)),
                    "VPBROADCASTMW2D lane %d", i);
}

static void test_inserts(void) {
    zmm_test_t base, out;
    ymm_t insert256;
    xmm_t insert128;
    for (int i = 0; i < 16; i++) base.u32[i] = UINT32_C(0x1000) + (uint32_t)i;
    for (int i = 0; i < 8; i++) insert256.u32[i] = UINT32_C(0x2000) + (uint32_t)i;
    for (int i = 0; i < 4; i++) insert128.u32[i] = UINT32_C(0x3000) + (uint32_t)i;

#define TEST_INSERT_128(INST, LABEL) do { \
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\t" #INST " $2, %2, %%zmm0, %%zmm1\n\t" \
                     "vmovdqa32 %%zmm1, %0" \
                     : "=m"(out) : "m"(base), "m"(insert128) : "zmm0", "zmm1"); \
    for (int i = 0; i < 16; i++) { \
        uint32_t expected = i >= 8 && i < 12 ? insert128.u32[i - 8] : base.u32[i]; \
        TEST_ASSERT(out.u32[i] == expected, LABEL " lane %d", i); \
    } \
} while (0)
    TEST_INSERT_128(vinsertf32x4, "VINSERTF32X4");
    TEST_INSERT_128(vinsertf64x2, "VINSERTF64X2");
    TEST_INSERT_128(vinserti64x2, "VINSERTI64X2");
#undef TEST_INSERT_128

#define TEST_INSERT_256(INST, LABEL) do { \
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\t" #INST " $1, %2, %%zmm0, %%zmm1\n\t" \
                     "vmovdqa32 %%zmm1, %0" \
                     : "=m"(out) : "m"(base), "m"(insert256) : "zmm0", "zmm1"); \
    for (int i = 0; i < 16; i++) { \
        uint32_t expected = i >= 8 ? insert256.u32[i - 8] : base.u32[i]; \
        TEST_ASSERT(out.u32[i] == expected, LABEL " lane %d", i); \
    } \
} while (0)
    TEST_INSERT_256(vinsertf32x8, "VINSERTF32X8");
    TEST_INSERT_256(vinsertf64x4, "VINSERTF64X4");
    TEST_INSERT_256(vinserti32x8, "VINSERTI32X8");
    TEST_INSERT_256(vinserti64x4, "VINSERTI64X4");
#undef TEST_INSERT_256
}

static void test_permi2_float(void) {
    zmm_test_t lo, hi, index, out;
    for (int i = 0; i < 16; i++) {
        lo.u32[i] = UINT32_C(0x3f000000) + (uint32_t)i;
        hi.u32[i] = UINT32_C(0x40000000) + (uint32_t)i;
        index.u32[i] = (uint32_t[]){0, 15, 16, 31, 32, 47, 63, UINT32_MAX,
                                    1, 14, 17, 30, 33, 46, 62, 64}[i];
    }
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t"
                     "vmovdqa32 %3, %%zmm2\n\tvpermi2ps %%zmm2, %%zmm0, %%zmm1\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(lo), "m"(index), "m"(hi)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) {
        unsigned n = index.u32[i] & 31U;
        TEST_ASSERT(out.u32[i] == (n < 16 ? lo.u32[n] : hi.u32[n - 16]),
                    "VPERMI2PS lane %d", i);
    }

    for (int i = 0; i < 8; i++) {
        lo.u64[i] = UINT64_C(0x3ff0000000000000) + (uint64_t)i;
        hi.u64[i] = UINT64_C(0x4000000000000000) + (uint64_t)i;
        index.u64[i] = (uint64_t[]){0, 7, 8, 15, 16, 23, UINT64_MAX, 32}[i];
    }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vmovdqa64 %3, %%zmm2\n\tvpermi2pd %%zmm2, %%zmm0, %%zmm1\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(lo), "m"(index), "m"(hi)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) {
        unsigned n = (unsigned)index.u64[i] & 15U;
        TEST_ASSERT(out.u64[i] == (n < 8 ? lo.u64[n] : hi.u64[n - 8]),
                    "VPERMI2PD lane %d", i);
    }
}

#define TEST_PERMT2(TYPE, LANES, BITS, MOVE, INST, LABEL) do { \
    zmm_test_t table0, table1, index, out; \
    for (int i = 0; i < (LANES); i++) { \
        table0.TYPE[i] = (typeof(table0.TYPE[i]))(0x10 + i); \
        table1.TYPE[i] = (typeof(table1.TYPE[i]))(0x80 + i); \
        index.TYPE[i] = (typeof(index.TYPE[i]))((i * 5) & ((LANES) * 2 - 1)); \
    } \
    __asm__ volatile(#MOVE " %1, %%zmm0\n\t" #MOVE " %2, %%zmm1\n\t" \
                     #MOVE " %3, %%zmm2\n\t" #INST " %%zmm1, %%zmm0, %%zmm2\n\t" \
                     #MOVE " %%zmm2, %0" \
                     : "=m"(out) : "m"(index), "m"(table1), "m"(table0) \
                     : "zmm0", "zmm1", "zmm2"); \
    for (int i = 0; i < (LANES); i++) { \
        unsigned n = (unsigned)index.TYPE[i] & ((LANES) * 2 - 1); \
        typeof(out.TYPE[i]) expected = n < (LANES) ? table0.TYPE[n] : table1.TYPE[n - (LANES)]; \
        TEST_ASSERT(out.TYPE[i] == expected, LABEL " lane %d", i); \
    } \
} while (0)

static void test_permt2(void) {
    TEST_PERMT2(u8, 64, 8, vmovdqu8, vpermt2b, "VPERMT2B");
    TEST_PERMT2(u16, 32, 16, vmovdqu16, vpermt2w, "VPERMT2W");
    TEST_PERMT2(u32, 16, 32, vmovdqu32, vpermt2ps, "VPERMT2PS");
    TEST_PERMT2(u64, 8, 64, vmovdqu64, vpermt2pd, "VPERMT2PD");
}

#undef TEST_PERMT2

int main(void) {
    TEST_START("Additional AVX-512 data movement and permutation forms");
    if (!CHECK_AVX512F()) return 0;
    test_broadcasts();
    test_inserts();
    test_permi2_float();
    test_permt2();
    TEST_END();
}
