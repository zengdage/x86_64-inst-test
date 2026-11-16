/* TARGET: VPTERNLOGD */
#include "../include/avx512_test.h"

static uint32_t ternary_ref(uint32_t a, uint32_t b, uint32_t c, unsigned imm) {
    uint32_t out = 0;
    for (unsigned bit = 0; bit < 32; bit++) {
        unsigned index = (((a >> bit) & 1U) << 2) |
                         (((b >> bit) & 1U) << 1) | ((c >> bit) & 1U);
        out |= ((imm >> index) & 1U) << bit;
    }
    return out;
}

int main(void) {
    TEST_START("VPTERNLOGD");
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 16; i++) {
        a.u32[i] = UINT32_C(0x01010101) * (uint32_t)(i + 1);
        b.u32[i] = UINT32_C(0xf0f00000) ^ (uint32_t)i;
        c.u32[i] = UINT32_C(0xaaaaaaaa) + (uint32_t)i;
    }
#define TEST_IMM(imm) do { \
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t" \
                     "vmovdqa32 %3, %%zmm2\n\t" \
                     "vpternlogd $" #imm ", %%zmm2, %%zmm1, %%zmm0\n\t" \
                     "vmovdqa32 %%zmm0, %0" \
                     : "=m"(out) : "m"(a), "m"(b), "m"(c) \
                     : "zmm0", "zmm1", "zmm2"); \
    for (int lane = 0; lane < 16; lane++) \
        TEST_ASSERT(out.u32[lane] == ternary_ref(a.u32[lane], b.u32[lane], \
                                                  c.u32[lane], imm), \
                    "VPTERNLOGD imm=0x%02x lane %d", imm, lane); \
} while (0)
    TEST_IMM(0x00);
    TEST_IMM(0xff);
    TEST_IMM(0x96);
    TEST_IMM(0xca);
#undef TEST_IMM

    uint32_t mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %4, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\tvmovdqa32 %3, %%zmm2\n\t"
                     "vpternlogd $0x96, %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (i == 0 || i == 15) ? a.u32[i] ^ b.u32[i] ^ c.u32[i] : 0;
        TEST_ASSERT(out.u32[i] == expected, "VPTERNLOGD zero mask lane %d", i);
    }
    TEST_END();
}
