/* TARGET: VP2INTERSECTD */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("VP2INTERSECTD");
    if (!CHECK_AVX512VP2INTERSECT()) return 0;
    zmm_test_t a, b;
    for (int i = 0; i < 16; i++) {
        a.u32[i] = (uint32_t)(i * 3);
        b.u32[i] = (uint32_t)(45 - i * 3);
    }
    a.u32[15] = 999;
    b.u32[15] = 999;
    uint32_t ma, mb;
    __asm__ volatile("vmovdqa32 %2, %%zmm0\n\tvmovdqa32 %3, %%zmm1\n\t"
                     "vp2intersectd %%zmm1, %%zmm0, %%k1\n\t"
                     "kmovw %%k1, %0\n\tkmovw %%k2, %1"
                     : "=r"(ma), "=r"(mb) : "m"(a), "m"(b)
                     : "zmm0", "zmm1", "k1", "k2");
    uint32_t ea = 0, eb = 0;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++)
            if (a.u32[i] == b.u32[j]) {
                ea |= 1U << i;
                eb |= 1U << j;
            }
    TEST_ASSERT((ma & UINT32_C(0xffff)) == ea, "VP2INTERSECTD first mask");
    TEST_ASSERT((mb & UINT32_C(0xffff)) == eb, "VP2INTERSECTD second mask");
    TEST_END();
}
