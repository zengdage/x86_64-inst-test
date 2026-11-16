/*
 * EVEX-encoded AVX-512 permute and shuffle instructions.
 *
 * Covers VPERMILPS, VPERMILPD, VPERMPD, VSHUFPS, VSHUFPD, VSHUFI32X4 and
 * VSHUFI64X2 with the 512 bit forms, including the lane local behaviour of
 * the shuffles and the merge and zero masking of the destinations.
 *
 * The operand order is the one used by the EVEX encodings: the first operand
 * is the memory capable source, the second operand is the register source
 * and the third operand is the destination.  The immediate forms take a
 * single source which is the first operand.
 */
#include "../include/avx512_test.h"

static zmm_test_t src1, src2, dst_init, out;

static void init_operands(void) {
    for (int i = 0; i < 16; i++) {
        src1.u32[i] = 0x100U + (uint32_t)i;
        src2.u32[i] = 0x200U + (uint32_t)i;
        dst_init.u32[i] = 0xdeadbeefU;
    }
}

/* Immediate form with a single source: source and destination. */
#define RUN_IMM2(ins, imm, out)                                                 \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %2, %%k1\n\t"                                                \
            "vmovdqa32 %1, %%zmm0\n\t"                                           \
            "vmovdqa32 %3, %%zmm2\n\t"                                          \
            ins " $" imm ", %%zmm0, %%zmm2%{%%k1%}\n\t"                          \
            "vmovdqa32 %%zmm2, %0"                                              \
            : "=m"(out)                                                         \
            : "m"(src2), "r"(mask), "m"(dst_init)                               \
            : "zmm0", "zmm2", "k1");                                             \
    } while (0)

/* Three operand form with two sources and a destination. */
#define RUN_3OP(ins, imm, out)                                                  \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %4, %%k1\n\t"                                                \
            "vmovdqa32 %1, %%zmm0\n\t"                                          \
            "vmovdqa32 %2, %%zmm1\n\t"                                          \
            "vmovdqa32 %3, %%zmm2\n\t"                                          \
            ins " $" imm ", %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"                  \
            "vmovdqa32 %%zmm2, %0"                                              \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "m"(dst_init), "r"(mask)                    \
            : "zmm0", "zmm1", "zmm2", "k1");                                     \
    } while (0)

static void test_permilps(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /* 0x1b selects the dwords in reverse order inside every 128 bit lane. */
    RUN_IMM2("vpermilps", "0x1b", out);
    for (int i = 0; i < 16; i++) {
        int lane = i & 3;
        uint32_t expected = src2.u32[(i & ~3) + (3 - lane)];
        TEST_ASSERT(out.u32[i] == expected, "VPERMILPS immediate lane %d", i);
    }

    /* 0xe4 selects the dwords in the original order. */
    RUN_IMM2("vpermilps", "0xe4", out);
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == src2.u32[i], "VPERMILPS identity lane %d", i);

    /* The variable form reads the control from the first operand and the
     * data from the second operand, one 128 bit lane at a time. */
    {
        zmm_test_t control;
        for (int i = 0; i < 16; i++) control.u32[i] = (uint32_t)(3 - (i & 3));
        __asm__ volatile(
            "kmovd %4, %%k1\n\t"
            "vmovdqa32 %1, %%zmm0\n\t"
            "vmovdqa32 %2, %%zmm1\n\t"
            "vmovdqa32 %3, %%zmm2\n\t"
            "vpermilps %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
            "vmovdqa32 %%zmm2, %0"
            : "=m"(out)
            : "m"(src1), "m"(control), "m"(dst_init), "r"(mask)
            : "zmm0", "zmm1", "zmm2", "k1");
        for (int i = 0; i < 16; i++) {
            int lane = i & 3;
            uint32_t expected = src1.u32[(i & ~3) + (3 - lane)];
            TEST_ASSERT(out.u32[i] == expected, "VPERMILPS variable lane %d", i);
        }
    }
}

static void test_permilpd(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /*
     * Every 128 bit lane owns two selector bits: imm8[2k] selects the low
     * qword of the lane and imm8[2k+1] the high qword.
     */
    const uint8_t imm = 0x5aU;
    RUN_IMM2("vpermilpd", "0x5a", out);
    for (int k = 0; k < 4; k++) {
        uint64_t low = src2.u64[2 * k + ((imm >> (2 * k)) & 1U)];
        uint64_t high = src2.u64[2 * k + ((imm >> (2 * k + 1)) & 1U)];
        TEST_ASSERT(out.u64[2 * k] == low, "VPERMILPD lane %d low qword", k);
        TEST_ASSERT(out.u64[2 * k + 1] == high, "VPERMILPD lane %d high qword", k);
    }

    /* 0xaa selects the second qword of every lane, which is the identity. */
    RUN_IMM2("vpermilpd", "0xaa", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == src2.u64[i], "VPERMILPD identity lane %d", i);
}

static void test_permpd(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /* The 512 bit form permutes the qwords inside every 256 bit lane. */
    RUN_IMM2("vpermpd", "0x1b", out);
    for (int i = 0; i < 8; i++) {
        int group = i & 3;
        uint64_t expected = src2.u64[(i & ~3) + (3 - group)];
        TEST_ASSERT(out.u64[i] == expected, "VPERMPD lane %d", i);
    }

    RUN_IMM2("vpermpd", "0xe4", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == src2.u64[i], "VPERMPD identity lane %d", i);
}

static void test_shufps(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /*
     * VSHUFPS selects two dwords from the register source and two from the
     * memory capable source inside every 128 bit lane.
     */
    RUN_3OP("vshufps", "0x1b", out);
    for (int lane = 0; lane < 4; lane++) {
        uint32_t base = (uint32_t)lane * 4;
        TEST_ASSERT(out.u32[base + 0] == src1.u32[base + 3], "VSHUFPS lane %d dword 0", lane);
        TEST_ASSERT(out.u32[base + 1] == src1.u32[base + 2], "VSHUFPS lane %d dword 1", lane);
        TEST_ASSERT(out.u32[base + 2] == src2.u32[base + 1], "VSHUFPS lane %d dword 2", lane);
        TEST_ASSERT(out.u32[base + 3] == src2.u32[base + 0], "VSHUFPS lane %d dword 3", lane);
    }

    RUN_3OP("vshufps", "0xe4", out);
    for (int lane = 0; lane < 4; lane++) {
        uint32_t base = (uint32_t)lane * 4;
        TEST_ASSERT(out.u32[base + 0] == src1.u32[base + 0], "VSHUFPS identity lane %d dword 0", lane);
        TEST_ASSERT(out.u32[base + 1] == src1.u32[base + 1], "VSHUFPS identity lane %d dword 1", lane);
        TEST_ASSERT(out.u32[base + 2] == src2.u32[base + 2], "VSHUFPS identity lane %d dword 2", lane);
        TEST_ASSERT(out.u32[base + 3] == src2.u32[base + 3], "VSHUFPS identity lane %d dword 3", lane);
    }
}

static void test_shufpd(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /*
     * Every 128 bit lane owns two selector bits: imm8[2k] selects the low
     * qword of the lane from the register source and imm8[2k+1] the high
     * qword from the memory capable source.
     */
    const uint8_t imm = 0x5aU;
    RUN_3OP("vshufpd", "0x5a", out);
    for (int k = 0; k < 4; k++) {
        uint64_t low = src1.u64[2 * k + ((imm >> (2 * k)) & 1U)];
        uint64_t high = src2.u64[2 * k + ((imm >> (2 * k + 1)) & 1U)];
        TEST_ASSERT(out.u64[2 * k] == low, "VSHUFPD lane %d low qword", k);
        TEST_ASSERT(out.u64[2 * k + 1] == high, "VSHUFPD lane %d high qword", k);
    }

    RUN_3OP("vshufpd", "0x00", out);
    for (int lane = 0; lane < 4; lane++) {
        int base = lane * 2;
        TEST_ASSERT(out.u64[base + 0] == src1.u64[base + 0], "VSHUFPD identity lane %d qword 0", lane);
        TEST_ASSERT(out.u64[base + 1] == src2.u64[base + 0], "VSHUFPD identity lane %d qword 1", lane);
    }
}

static void test_shufi(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /*
     * VSHUFI32X4 takes the low two 128 bit lanes from the register source
     * and the high two from the memory capable source.
     */
    RUN_3OP("vshufi32x4", "0x1b", out);
    for (int lane = 0; lane < 4; lane++) {
        int selector = (lane == 0) ? 3 : (lane == 1) ? 2 : (lane == 2) ? 1 : 0;
        const zmm_test_t *source = (lane < 2) ? &src1 : &src2;
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(out.u32[lane * 4 + i] == source->u32[selector * 4 + i],
                        "VSHUFI32X4 lane %d dword %d", lane, i);
    }

    RUN_3OP("vshufi64x2", "0x1b", out);
    for (int lane = 0; lane < 4; lane++) {
        int selector = (lane == 0) ? 3 : (lane == 1) ? 2 : (lane == 2) ? 1 : 0;
        const zmm_test_t *source = (lane < 2) ? &src1 : &src2;
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(out.u32[lane * 4 + i] == source->u32[selector * 4 + i],
                        "VSHUFI64X2 lane %d dword %d", lane, i);
    }
}

static void test_masking(void) {
    uint32_t mask;

    init_operands();

    /* Merge masking keeps the destination lanes that are masked out. */
    mask = 0x0003U;
    __asm__ volatile(
        "kmovd %4, %%k1\n\t"
        "vmovdqa32 %1, %%zmm0\n\t"
        "vmovdqa32 %2, %%zmm1\n\t"
        "vmovdqa32 %3, %%zmm2\n\t"
        "vshufps $0x1b, %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovdqa32 %%zmm2, %0"
        : "=m"(out)
        : "m"(src1), "m"(src2), "m"(dst_init), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    TEST_ASSERT(out.u32[0] == src1.u32[3], "VSHUFPS masked lane 0");
    TEST_ASSERT(out.u32[1] == src1.u32[2], "VSHUFPS masked lane 1");
    TEST_ASSERT(out.u32[2] == dst_init.u32[2], "VSHUFPS merge keeps lane 2");
    TEST_ASSERT(out.u32[15] == dst_init.u32[15], "VSHUFPS merge keeps lane 15");

    /* Zero masking clears the masked out lanes. */
    __asm__ volatile(
        "kmovd %4, %%k1\n\t"
        "vmovdqa32 %1, %%zmm0\n\t"
        "vmovdqa32 %2, %%zmm1\n\t"
        "vmovdqa32 %3, %%zmm2\n\t"
        "vshufps $0x1b, %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqa32 %%zmm2, %0"
        : "=m"(out)
        : "m"(src1), "m"(src2), "m"(dst_init), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    TEST_ASSERT(out.u32[0] == src1.u32[3], "VSHUFPS zero mask lane 0");
    TEST_ASSERT(out.u32[2] == 0U, "VSHUFPS zero mask clears lane 2");
    TEST_ASSERT(out.u32[15] == 0U, "VSHUFPS zero mask clears lane 15");
}

int main(void) {
    TEST_START("EVEX AVX-512 permute and shuffle instructions");
    if (!CHECK_AVX512F()) return 0;
    test_permilps();
    test_permilpd();
    test_permpd();
    test_shufps();
    test_shufpd();
    test_shufi();
    test_masking();
    TEST_END();
}
