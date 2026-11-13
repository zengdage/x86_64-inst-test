/* AVX-512BF16 conversion and dot-product instructions. */
#include "avx512_test.h"

static uint16_t float_to_bf16(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    uint32_t rounded = bits + UINT32_C(0x7fff) + ((bits >> 16) & 1U);
    return (uint16_t)(rounded >> 16);
}

int main(void) {
    TEST_START("AVX-512BF16 conversion and dot product");
    if (!CHECK_AVX512BF16()) return 0;
    zmm_test_t a, b, paired, dot;
    ymm_t single;
    for (int i = 0; i < 16; i++) {
        a.f32[i] = (float)(i - 8) + 0.25f;
        b.f32[i] = (float)(16 - i) + 0.5f;
    }
    __asm__ volatile("vmovaps %3, %%zmm0\n\tvmovaps %4, %%zmm1\n\t"
                     "vcvtneps2bf16 %%zmm0, %%ymm2\n\t"
                     "vcvtne2ps2bf16 %%zmm1, %%zmm0, %%zmm3\n\t"
                     "vmovdqu %%ymm2, %0\n\tvmovdqa64 %%zmm3, %1\n\t"
                     "vxorps %%zmm4, %%zmm4, %%zmm4\n\t"
                     "vdpbf16ps %%zmm3, %%zmm3, %%zmm4\n\tvmovaps %%zmm4, %2"
                     : "=m"(single), "=m"(paired), "=m"(dot)
                     : "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(single.u16[i] == float_to_bf16(a.f32[i]), "VCVTNEPS2BF16 lane %d", i);
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(paired.u16[i] == float_to_bf16(b.f32[i]), "VCVTNE2PS2BF16 low lane %d", i);
        TEST_ASSERT(paired.u16[16 + i] == float_to_bf16(a.f32[i]), "VCVTNE2PS2BF16 high lane %d", i);
    }
    for (int i = 0; i < 16; i++) {
        float lo = (float)(int32_t)0;
        uint32_t bits0 = (uint32_t)paired.u16[i * 2] << 16;
        uint32_t bits1 = (uint32_t)paired.u16[i * 2 + 1] << 16;
        float f0, f1;
        memcpy(&f0, &bits0, 4); memcpy(&f1, &bits1, 4);
        lo = f0 * f0 + f1 * f1;
        TEST_ASSERT(dot.f32[i] == lo, "VDPBF16PS lane %d", i);
    }
    TEST_END();
}
