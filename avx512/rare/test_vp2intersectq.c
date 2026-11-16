/* TARGET: VP2INTERSECTQ */
#include "../include/avx512_test.h"

int main(void) {
    TEST_START("VP2INTERSECTQ");
    if (!CHECK_AVX512VP2INTERSECT()) return 0;
    zmm_test_t a = {.u64 = {0, 1, 2, 3, 4, 5, 6, UINT64_MAX}};
    zmm_test_t b = {.u64 = {9, 5, 8, 3, 7, 1, UINT64_MAX, 0}};
    uint32_t ma, mb;
    __asm__ volatile("vmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
                     "vp2intersectq %%zmm1, %%zmm0, %%k1\n\t"
                     "kmovb %%k1, %0\n\tkmovb %%k2, %1"
                     : "=r"(ma), "=r"(mb) : "m"(a), "m"(b)
                     : "zmm0", "zmm1", "k1", "k2");
    uint32_t ea = 0, eb = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if (a.u64[i] == b.u64[j]) {
                ea |= 1U << i;
                eb |= 1U << j;
            }
    TEST_ASSERT((ma & UINT32_C(0xff)) == ea, "VP2INTERSECTQ first mask");
    TEST_ASSERT((mb & UINT32_C(0xff)) == eb, "VP2INTERSECTQ second mask");
    TEST_END();
}
