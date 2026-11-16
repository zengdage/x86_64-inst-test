/* TARGET: VPMADD52HUQ */
#include "../include/avx512_test.h"

#define MASK52 ((UINT64_C(1) << 52) - 1)

static uint64_t reference(uint64_t acc, uint64_t a, uint64_t b) {
    unsigned __int128 product = (unsigned __int128)(a & MASK52) * (b & MASK52);
    return acc + (uint64_t)(product >> 52);
}

int main(void) {
    TEST_START("VPMADD52HUQ");
    if (!CHECK_AVX512IFMA()) return 0;
    zmm_test_t acc, a, b, out;
    for (int i = 0; i < 8; i++) {
        acc.u64[i] = UINT64_MAX - (uint64_t)i;
        a.u64[i] = (MASK52 - (uint64_t)i) | (UINT64_C(0xfff) << 52);
        b.u64[i] = UINT64_C(0xabcdef0123456) + (uint64_t)i;
    }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vmovdqa64 %3, %%zmm2\n\t"
                     "vpmadd52huq %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(acc), "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == reference(acc.u64[i], a.u64[i], b.u64[i]),
                    "VPMADD52HUQ lane %d", i);
    TEST_END();
}
