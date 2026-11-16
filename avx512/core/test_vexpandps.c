/* TARGET: VEXPANDPS */
#include "../include/avx512_test.h"

static int selected(uint32_t mask, int lane) {
    return (int)((mask >> lane) & 1U);
}

int main(void) {
    TEST_START("VEXPANDPS");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t src, dst;
    for (int i = 0; i < 16; i++) src.f32[i] = (float)i + 0.5f;
    const uint32_t mask = UINT32_C(0x8429);
    __asm__ volatile("kmovd %2, %%k1\n\t"
                     "vmovaps %1, %%zmm0\n\t"
                     "vexpandps %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovaps %%zmm1, %0"
                     : "=m"(dst) : "m"(src), "r"(mask) : "zmm0", "zmm1", "k1");
    int consumed = 0;
    for (int i = 0; i < 16; i++) {
        float expected = selected(mask, i) ? src.f32[consumed++] : 0.0f;
        TEST_ASSERT(dst.f32[i] == expected, "VEXPANDPS lane %d", i);
    }
    TEST_ASSERT(consumed == 5, "VEXPANDPS consumed %d elements", consumed);
    TEST_END();
}
