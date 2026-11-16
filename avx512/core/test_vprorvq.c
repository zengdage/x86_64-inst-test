/* AVX-512F variable qword rotate right. */
#include "../include/avx512_test.h"

static uint64_t reference(uint64_t value, unsigned count) {
    count &= 63U;
    return count ? (value >> count) | (value << (64U - count)) : value;
}

int main(void) {
    TEST_START("AVX-512F VPRORVQ");
    if (!CHECK_AVX512F()) return 0;
    static const uint64_t counts[8] = {0, 1, 31, 63, 64, 65, 127, UINT64_MAX};
    zmm_test_t value, count, out, zeroed;
    for (int i = 0; i < 8; i++) {
        value.u64[i] = UINT64_C(0x8123456789abcdef) ^ ((uint64_t)i << 52);
        count.u64[i] = counts[i];
    }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vprorvq %%zmm1, %%zmm0, %%zmm2\n\tvmovdqa64 %%zmm2, %0"
                     : "=m"(out) : "m"(value), "m"(count)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == reference(value.u64[i], (unsigned)counts[i]),
                    "VPRORVQ lane %d count %llu", i, (unsigned long long)counts[i]);
    const uint32_t mask = 0x81U;
    __asm__ volatile("kmovb %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\tvprorvq %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm2, %0"
                     : "=m"(zeroed) : "m"(value), "m"(count), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = ((mask >> i) & 1U)
            ? reference(value.u64[i], (unsigned)counts[i]) : 0;
        TEST_ASSERT(zeroed.u64[i] == expected, "VPRORVQ zero mask lane %d", i);
    }
    TEST_END();
}
