/* Generic AVX-512 signed/unsigned integer comparisons and all predicates. */
#include "../include/avx512_test.h"

static int signed_predicate(int64_t a, int64_t b, unsigned predicate) {
    switch (predicate & 7U) {
    case 0: return a == b;
    case 1: return a < b;
    case 2: return a <= b;
    case 3: return 0;
    case 4: return a != b;
    case 5: return a >= b;
    case 6: return a > b;
    default: return 1;
    }
}

static int unsigned_predicate(uint64_t a, uint64_t b, unsigned predicate) {
    switch (predicate & 7U) {
    case 0: return a == b;
    case 1: return a < b;
    case 2: return a <= b;
    case 3: return 0;
    case 4: return a != b;
    case 5: return a >= b;
    case 6: return a > b;
    default: return 1;
    }
}

static uint32_t signed_dword_mask(const zmm_test_t *a, const zmm_test_t *b, unsigned pred) {
    uint32_t mask = 0;
    for (int i = 0; i < 16; i++) mask |= (uint32_t)signed_predicate(a->i32[i], b->i32[i], pred) << i;
    return mask;
}

static uint32_t unsigned_dword_mask(const zmm_test_t *a, const zmm_test_t *b, unsigned pred) {
    uint32_t mask = 0;
    for (int i = 0; i < 16; i++) mask |= (uint32_t)unsigned_predicate(a->u32[i], b->u32[i], pred) << i;
    return mask;
}

static void test_dword_predicates(void) {
    zmm_test_t a, b;
    const uint32_t av[16] = {
        0, 1, UINT32_MAX, 0x80000000U, 0x7fffffffU, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15
    };
    const uint32_t bv[16] = {
        0, 2, 0, 0x7fffffffU, 0x80000000U, 5, 5, 8,
        7, 10, 10, 10, 13, 12, 15, 14
    };
    memcpy(a.u32, av, sizeof(av));
    memcpy(b.u32, bv, sizeof(bv));
    uint32_t got;
#define TEST_PCMPD(P) do { \
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t" \
                     "vpcmpd $" #P ", %%zmm1, %%zmm0, %%k1\n\tkmovw %%k1, %0" \
                     : "=r"(got) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1"); \
    TEST_ASSERT((got & 0xffffU) == signed_dword_mask(&a, &b, P), \
                "VPCMPD predicate %d: 0x%04x", P, got); \
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t" \
                     "vpcmpud $" #P ", %%zmm1, %%zmm0, %%k1\n\tkmovw %%k1, %0" \
                     : "=r"(got) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1"); \
    TEST_ASSERT((got & 0xffffU) == unsigned_dword_mask(&a, &b, P), \
                "VPCMPUD predicate %d: 0x%04x", P, got); \
} while (0)
    TEST_PCMPD(0); TEST_PCMPD(1); TEST_PCMPD(2); TEST_PCMPD(3);
    TEST_PCMPD(4); TEST_PCMPD(5); TEST_PCMPD(6); TEST_PCMPD(7);
#undef TEST_PCMPD

    uint32_t writemask = 0x8001U;
    __asm__ volatile(
        "kmovw %3, %%k2\n\tvmovdqa32 %1, %%zmm0\n\t"
        "vmovdqa32 %2, %%zmm1\n\tvpcmpd $7, %%zmm1, %%zmm0, %%k1%{%%k2%}\n\t"
        "kmovw %%k1, %0"
        : "=r"(got) : "m"(a), "m"(b), "r"(writemask)
        : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT((got & 0xffffU) == writemask,
                "VPCMPD writemask clears unselected result bits");
}

static void test_qword_byte_word_boundaries(void) {
    zmm_test_t a, b;
    for (int i = 0; i < 8; i++) {
        a.u64[i] = i == 7 ? UINT64_MAX : (uint64_t)i;
        b.u64[i] = i == 0 ? UINT64_MAX : (uint64_t)(7 - i);
    }
    uint32_t got;
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vpcmpuq $1, %%zmm1, %%zmm0, %%k1\n\tkmovb %%k1, %0"
                     : "=r"(got) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    uint32_t expected = 0;
    for (int i = 0; i < 8; i++) expected |= (uint32_t)(a.u64[i] < b.u64[i]) << i;
    TEST_ASSERT((got & 0xffU) == expected, "VPCMPUQ unsigned boundary mask");

    for (int i = 0; i < 64; i++) {
        a.i8[i] = (int8_t)(i - 32);
        b.i8[i] = (int8_t)(31 - i);
    }
    uint64_t got64;
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vpcmpb $6, %%zmm1, %%zmm0, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(got64) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    uint64_t expected64 = 0;
    for (int i = 0; i < 64; i++) expected64 |= (uint64_t)(a.i8[i] > b.i8[i]) << i;
    TEST_ASSERT(got64 == expected64, "VPCMPB signed first/high mask boundaries");

    for (int i = 0; i < 32; i++) {
        a.u16[i] = (uint16_t)(i * 2000U);
        b.u16[i] = (uint16_t)(UINT16_MAX - i * 2000U);
    }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vpcmpuw $2, %%zmm1, %%zmm0, %%k1\n\tkmovd %%k1, %0"
                     : "=r"(got) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    expected = 0;
    for (int i = 0; i < 32; i++) expected |= (uint32_t)(a.u16[i] <= b.u16[i]) << i;
    TEST_ASSERT(got == expected, "VPCMPUW unsigned word wrap boundaries");
}

int main(void) {
    TEST_START("VPCMP/VPCMPU AVX-512 predicates");
    if (!CHECK_AVX512F()) return 0;
    test_dword_predicates();
    test_qword_byte_word_boundaries();
    TEST_END();
}
