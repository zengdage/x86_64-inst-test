/* AVX-512VBMI VPMULTISHIFTQB control boundaries, masks, and overlap. */
#include "avx512_test.h"

static uint8_t multishift_ref(uint8_t control, uint64_t data) {
    unsigned shift = control & 63U;
    uint64_t rotated = shift == 0 ? data : (data >> shift) | (data << (64 - shift));
    return (uint8_t)rotated;
}

static void fill_inputs(zmm_test_t *control, zmm_test_t *data) {
    static const uint8_t shifts[8] = {0, 1, 7, 8, 31, 32, 56, 63};
    for (int q = 0; q < 8; q++) {
        data->u64[q] = UINT64_C(0x0123456789abcdef) ^
                       (UINT64_C(0x1111111111111111) * (uint64_t)q);
        for (int j = 0; j < 8; j++)
            control->u8[q * 8 + j] = shifts[j] | (uint8_t)(q << 6);
    }
}

static void test_controls(void) {
    zmm_test_t control, data, out;
    fill_inputs(&control, &data);
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
        "vpmultishiftqb %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqa64 %%zmm2, %0"
        : "=m"(out) : "m"(control), "m"(data) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 64; i++) {
        uint8_t expected = multishift_ref(control.u8[i], data.u64[i / 8]);
        TEST_ASSERT(out.u8[i] == expected,
                    "VPMULTISHIFTQB byte %d: 0x%02x != 0x%02x",
                    i, out.u8[i], expected);
    }
}

static void test_masks_and_aliases(void) {
    zmm_test_t control, data, merge, out;
    fill_inputs(&control, &data);
    memset(&merge, 0xa5, sizeof(merge));
    uint64_t mask = UINT64_C(0x8000000000000001);
    __asm__ volatile(
        "kmovq %4, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
        "vmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
        "vpmultishiftqb %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovdqa64 %%zmm2, %0"
        : "=m"(out) : "m"(control), "m"(data), "m"(merge), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 64; i++) {
        uint8_t expected = (i == 0 || i == 63)
                           ? multishift_ref(control.u8[i], data.u64[i / 8]) : 0xa5;
        TEST_ASSERT(out.u8[i] == expected, "VPMULTISHIFTQB merge mask byte %d", i);
    }

    /* Control and destination alias; source data remains distinct. */
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
        "vpmultishiftqb %%zmm1, %%zmm0, %%zmm0\n\t"
        "vmovdqa64 %%zmm0, %0"
        : "=m"(out) : "m"(control), "m"(data) : "zmm0", "zmm1");
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(out.u8[i] == multishift_ref(control.u8[i], data.u64[i / 8]),
                    "VPMULTISHIFTQB destination overlap byte %d", i);
}

int main(void) {
    TEST_START("VPMULTISHIFTQB AVX-512VBMI");
    if (!CHECK_AVX512VBMI()) return 0;
    test_controls();
    test_masks_and_aliases();
    TEST_END();
}
