/* VPTERNLOGD/Q truth tables, masking, and destructive destination aliasing. */
#include "avx512_test.h"

static uint32_t ternary_ref(uint32_t a, uint32_t b, uint32_t c, unsigned imm) {
    uint32_t out = 0;
    for (unsigned bit = 0; bit < 32; bit++) {
        unsigned index = (((a >> bit) & 1U) << 2) |
                         (((b >> bit) & 1U) << 1) | ((c >> bit) & 1U);
        out |= ((imm >> index) & 1U) << bit;
    }
    return out;
}

static void test_dword_truth_tables(void) {
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 16; i++) {
        a.u32[i] = 0x01010101U * (uint32_t)(i + 1);
        b.u32[i] = 0xf0f00000U ^ (uint32_t)i;
        c.u32[i] = 0xaaaaaaaaU + (uint32_t)i;
    }
#define TEST_IMM(imm) do { \
    __asm__ volatile( \
        "vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t" \
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
    TEST_IMM(0x96); /* XOR of all three operands. */
    TEST_IMM(0xca); /* Bitwise select: a ? b : c. */
#undef TEST_IMM

    uint32_t kmask = 0x8001U;
    __asm__ volatile(
        "kmovw %4, %%k1\n\t"
        "vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t"
        "vmovdqa32 %3, %%zmm2\n\t"
        "vpternlogd $0x96, %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
        "vmovdqa32 %%zmm0, %0"
        : "=m"(out) : "m"(a), "m"(b), "m"(c), "r"(kmask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (i == 0 || i == 15)
                            ? (a.u32[i] ^ b.u32[i] ^ c.u32[i]) : 0;
        TEST_ASSERT(out.u32[i] == expected, "VPTERNLOGD zero mask lane %d", i);
    }
}

static void test_qword_aliases(void) {
    zmm_test_t a, b, c, out;
    for (int i = 0; i < 8; i++) {
        a.u64[i] = UINT64_C(0x1111111111111111) * (uint64_t)(i + 1);
        b.u64[i] = UINT64_C(0xff00ff00ff00ff00) ^ (uint64_t)i;
        c.u64[i] = UINT64_C(0xaaaaaaaaaaaaaaaa) + (uint64_t)i;
    }
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
        "vmovdqa64 %3, %%zmm2\n\t"
        "vpternlogq $0x96, %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovdqa64 %%zmm0, %0"
        : "=m"(out) : "m"(a), "m"(b), "m"(c)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (a.u64[i] ^ b.u64[i] ^ c.u64[i]),
                    "VPTERNLOGQ destructive destination lane %d", i);

    /* All three logical operands occupy the same architectural register. */
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\t"
        "vpternlogq $0x96, %%zmm0, %%zmm0, %%zmm0\n\t"
        "vmovdqa64 %%zmm0, %0"
        : "=m"(out) : "m"(a) : "zmm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == a.u64[i], "VPTERNLOGQ all operands aliased lane %d", i);
}

int main(void) {
    TEST_START("VPTERNLOGD/VPTERNLOGQ AVX-512");
    if (!CHECK_AVX512F()) return 0;
    test_dword_truth_tables();
    test_qword_aliases();
    TEST_END();
}
