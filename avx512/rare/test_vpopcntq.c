/* TARGET: VPOPCNTQ */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("VPOPCNTQ");
    if (!CHECK_AVX512VPOPCNTDQ()) return 0;
    zmm_test_t src, out;
    for (int i = 0; i < 8; i++)
        src.u64[i] = UINT64_C(0x0102040810204081) * (uint64_t)(i + 1) ^
                     (i == 7 ? UINT64_MAX : 0);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvpopcntq %%zmm0, %%zmm1\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (uint64_t)__builtin_popcountll(src.u64[i]),
                    "VPOPCNTQ lane %d", i);
    TEST_END();
}
