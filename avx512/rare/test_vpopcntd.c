/* TARGET: VPOPCNTD */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("VPOPCNTD");
    if (!CHECK_AVX512VPOPCNTDQ()) return 0;
    zmm_test_t src, out;
    for (int i = 0; i < 16; i++)
        src.u32[i] = UINT32_C(0x01020408) * (uint32_t)(i + 1) ^
                     (i == 15 ? UINT32_MAX : 0);
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvpopcntd %%zmm0, %%zmm1\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == (uint32_t)__builtin_popcount(src.u32[i]),
                    "VPOPCNTD lane %d", i);
    TEST_END();
}
