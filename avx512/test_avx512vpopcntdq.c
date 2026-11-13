/* AVX-512VPOPCNTDQ packed dword/qword population counts. */
#include "avx512_test.h"

int main(void) {
    TEST_START("AVX-512VPOPCNTDQ VPOPCNTD/Q");
    if (!CHECK_AVX512VPOPCNTDQ()) return 0;
    zmm_test_t src, dword, qword;
    for (int i = 0; i < 8; i++)
        src.u64[i] = (UINT64_C(0x0102040810204081) * (uint64_t)(i + 1)) ^
                     (i == 7 ? UINT64_MAX : 0);
    __asm__ volatile("vmovdqa64 %2, %%zmm0\n\tvpopcntd %%zmm0, %%zmm1\n\t"
                     "vpopcntq %%zmm0, %%zmm2\n\tvmovdqa32 %%zmm1, %0\n\t"
                     "vmovdqa64 %%zmm2, %1"
                     : "=m"(dword), "=m"(qword) : "m"(src)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dword.u32[i] == (uint32_t)__builtin_popcount(src.u32[i]),
                    "VPOPCNTD lane %d", i);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(qword.u64[i] == (uint64_t)__builtin_popcountll(src.u64[i]),
                    "VPOPCNTQ lane %d", i);
    TEST_END();
}
