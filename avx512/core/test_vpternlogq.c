/* TARGET: VPTERNLOGQ */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("VPTERNLOGQ");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 8; i++) {
        a.u64[i] = UINT64_C(0x1111111111111111) * (uint64_t)(i + 1);
        b.u64[i] = UINT64_C(0xff00ff00ff00ff00) ^ (uint64_t)i;
        c.u64[i] = UINT64_C(0xaaaaaaaaaaaaaaaa) + (uint64_t)i;
    }
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vmovdqa64 %3, %%zmm2\n\t"
                     "vpternlogq $0x96, %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (a.u64[i] ^ b.u64[i] ^ c.u64[i]),
                    "VPTERNLOGQ lane %d", i);

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\t"
                     "vpternlogq $0x96, %%zmm0, %%zmm0, %%zmm0\n\t"
                     "vmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a) : "zmm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == a.u64[i], "VPTERNLOGQ aliased lane %d", i);
    TEST_END();
}
