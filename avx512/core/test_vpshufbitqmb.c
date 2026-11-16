/* TARGET: VPSHUFBITQMB */
#include "../include/avx512_test.h"

static uint64_t expected_result(const zmm_test_t *data,
                                const zmm_test_t *control) {
    uint64_t expected = 0;
    for (int i = 0; i < 64; i++)
        expected |= ((data->u64[i / 8] >> (control->u8[i] & 63U)) & 1U) << i;
    return expected;
}

int main(void) {
    zmm_test_t data, control;
    uint64_t result, expected, writemask;

    TEST_START("VPSHUFBITQMB");
    if (!CHECK_AVX512BITALG()) return 0;
    for (int q = 0; q < 8; q++) {
        data.u64[q] = UINT64_C(0x0123456789abcdef) ^
                      (UINT64_C(0x1111111111111111) * (uint64_t)q);
        for (int j = 0; j < 8; j++)
            control.u8[q * 8 + j] = (uint8_t)((j * 9 + q) & 63);
    }
    expected = expected_result(&data, &control);

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vpshufbitqmb %%zmm1, %%zmm0, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(result) : "m"(data), "m"(control)
                     : "zmm0", "zmm1", "k1");
    TEST_ASSERT(result == expected, "VPSHUFBITQMB result 0x%016" PRIx64,
                result);

    writemask = UINT64_C(0x8000000000000001);
    __asm__ volatile("kmovq %3, %%k2\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpshufbitqmb %%zmm1, %%zmm0, %%k1%{%%k2%}\n\t"
                     "kmovq %%k1, %0"
                     : "=r"(result)
                     : "m"(data), "m"(control), "r"(writemask)
                     : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT(result == (expected & writemask),
                "VPSHUFBITQMB endpoint writemask");

    writemask = UINT64_C(0xa55aa55aa55aa55a);
    __asm__ volatile("kmovq %3, %%k2\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     "vpshufbitqmb %%zmm1, %%zmm0, %%k1%{%%k2%}\n\t"
                     "kmovq %%k1, %0"
                     : "=r"(result)
                     : "m"(data), "m"(control), "r"(writemask)
                     : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT(result == (expected & writemask),
                "VPSHUFBITQMB mixed writemask");
    TEST_END();
}
