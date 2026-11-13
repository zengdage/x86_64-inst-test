/* AVX-512IFMA low/high 52-bit multiply-add with a 104-bit reference. */
#include "avx512_test.h"

#define MASK52 ((UINT64_C(1) << 52) - 1)

static uint64_t madd52lo(uint64_t acc, uint64_t a, uint64_t b) {
    unsigned __int128 product = (unsigned __int128)(a & MASK52) * (b & MASK52);
    return acc + ((uint64_t)product & MASK52);
}

static uint64_t madd52hi(uint64_t acc, uint64_t a, uint64_t b) {
    unsigned __int128 product = (unsigned __int128)(a & MASK52) * (b & MASK52);
    return acc + (uint64_t)(product >> 52);
}

static void test_low_high(void) {
    zmm_test_t acc, a, b, low, high;
    const uint64_t values[8] = {
        0, 1, MASK52, MASK52 - 1, UINT64_C(0x123456789abcd),
        UINT64_C(1) << 51, UINT64_MAX, UINT64_C(0xabcdef0123456)
    };
    for (int i = 0; i < 8; i++) {
        acc.u64[i] = UINT64_MAX - (uint64_t)i;
        a.u64[i] = values[i];
        b.u64[i] = values[7 - i];
    }
    __asm__ volatile(
        "vmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
        "vmovdqa64 %4, %%zmm2\n\t"
        "vpmadd52luq %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovdqa64 %%zmm0, %0\n\t"
        "vmovdqa64 %2, %%zmm0\n\t"
        "vpmadd52huq %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovdqa64 %%zmm0, %1"
        : "=m"(low), "=m"(high) : "m"(acc), "m"(a), "m"(b)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(low.u64[i] == madd52lo(acc.u64[i], a.u64[i], b.u64[i]),
                    "VPMADD52LUQ lane %d", i);
        TEST_ASSERT(high.u64[i] == madd52hi(acc.u64[i], a.u64[i], b.u64[i]),
                    "VPMADD52HUQ lane %d", i);
    }

    /* Bits above bit 51 of both multiplicands are architecturally ignored. */
    for (int i = 0; i < 8; i++) {
        a.u64[i] = UINT64_MAX;
        b.u64[i] = UINT64_MAX;
        acc.u64[i] = 0;
    }
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
        "vmovdqa64 %3, %%zmm2\n\t"
        "vpmadd52huq %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovdqa64 %%zmm0, %0"
        : "=m"(high) : "m"(acc), "m"(a), "m"(b)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(high.u64[i] == madd52hi(0, UINT64_MAX, UINT64_MAX),
                    "VPMADD52HUQ ignores upper operand bits lane %d", i);
}

static void test_masks_and_aliases(void) {
    zmm_test_t acc, a, b, out;
    for (int i = 0; i < 8; i++) {
        acc.u64[i] = 1000U + (uint64_t)i;
        a.u64[i] = 3U + (uint64_t)i;
        b.u64[i] = 7U + (uint64_t)i;
    }
    uint32_t mask = 0x81U;
    __asm__ volatile(
        "kmovb %4, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
        "vmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
        "vpmadd52luq %%zmm2, %%zmm1, %%zmm0%{%%k1%}\n\t"
        "vmovdqa64 %%zmm0, %0"
        : "=m"(out) : "m"(acc), "m"(a), "m"(b), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (i == 0 || i == 7)
                            ? madd52lo(acc.u64[i], a.u64[i], b.u64[i]) : acc.u64[i];
        TEST_ASSERT(out.u64[i] == expected, "VPMADD52LUQ merge mask lane %d", i);
    }

    /* Destination, first source, and accumulator are the same register. */
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
        "vpmadd52luq %%zmm1, %%zmm0, %%zmm0\n\t"
        "vmovdqa64 %%zmm0, %0"
        : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == madd52lo(a.u64[i], a.u64[i], b.u64[i]),
                    "VPMADD52LUQ source/destination overlap lane %d", i);
}

int main(void) {
    TEST_START("VPMADD52LUQ/VPMADD52HUQ AVX-512IFMA");
    if (!CHECK_AVX512IFMA()) return 0;
    test_low_high();
    test_masks_and_aliases();
    TEST_END();
}
