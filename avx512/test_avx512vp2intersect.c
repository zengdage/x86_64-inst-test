/* AVX-512VP2INTERSECT dword/qword set intersection masks. */
#include "avx512_test.h"

static void test_dword(void) {
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
    for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++)
        if (a.u32[i] == b.u32[j]) { ea |= 1U << i; eb |= 1U << j; }
    TEST_ASSERT((ma & 0xffffU) == ea, "VP2INTERSECTD first mask");
    TEST_ASSERT((mb & 0xffffU) == eb, "VP2INTERSECTD second mask");
}

static void test_qword(void) {
    zmm_test_t a = {.u64 = {0, 1, 2, 3, 4, 5, 6, UINT64_MAX}};
    zmm_test_t b = {.u64 = {9, 5, 8, 3, 7, 1, UINT64_MAX, 0}};
    uint32_t ma, mb;
    __asm__ volatile("vmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
                     "vp2intersectq %%zmm1, %%zmm0, %%k1\n\t"
                     "kmovb %%k1, %0\n\tkmovb %%k2, %1"
                     : "=r"(ma), "=r"(mb) : "m"(a), "m"(b)
                     : "zmm0", "zmm1", "k1", "k2");
    uint32_t ea = 0, eb = 0;
    for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++)
        if (a.u64[i] == b.u64[j]) { ea |= 1U << i; eb |= 1U << j; }
    TEST_ASSERT((ma & 0xffU) == ea, "VP2INTERSECTQ first mask");
    TEST_ASSERT((mb & 0xffU) == eb, "VP2INTERSECTQ second mask");
}

int main(void) {
    TEST_START("AVX-512VP2INTERSECT");
    if (!CHECK_AVX512VP2INTERSECT()) return 0;
    test_dword();
    test_qword();
    TEST_END();
}
