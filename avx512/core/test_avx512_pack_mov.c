/*
 * EVEX-encoded AVX-512 unpack, pack, shift and bitwise instructions.
 *
 * Covers VUNPCKLPS, VUNPCKHPS, VUNPCKLPD, VUNPCKHPD, VPACKSSDW,
 * VPACKUSDW, VPSLLDQ, VPSRLDQ, VPORQ, VPANDQ and VPANDNQ with the 512 bit
 * forms, including the saturation of the packing instructions and the
 * 128 bit lane local behaviour of the byte shifts.
 */
#include "../include/avx512_test.h"
#include <limits.h>

static zmm_test_t src1, src2, dst_init, out;

static void init_operands(void) {
    for (int i = 0; i < 16; i++) {
        src1.u32[i] = 0x11111111U * (uint32_t)(i + 1);
        src2.u32[i] = 0x22222222U * (uint32_t)(i + 2);
        dst_init.u32[i] = 0xdeadbeefU;
    }
}

/* Three operand form: two sources and the destination. */
#define RUN_3OP(ins, out)                                                       \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovdqa32 %1, %%zmm0\n\t"                                          \
            "vmovdqa32 %2, %%zmm1\n\t"                                          \
            "vmovdqa32 %4, %%zmm2\n\t"                                          \
            ins " %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"                           \
            "vmovdqa32 %%zmm2, %0"                                              \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "zmm0", "zmm1", "zmm2", "k1");                                     \
    } while (0)

/* Three operand form with an immediate. */
#define RUN_3OP_IMM(ins, imm, out)                                              \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "kmovd %3, %%k1\n\t"                                                \
            "vmovdqa32 %1, %%zmm0\n\t"                                          \
            "vmovdqa32 %2, %%zmm1\n\t"                                          \
            "vmovdqa32 %4, %%zmm2\n\t"                                          \
            ins " $" imm ", %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"                  \
            "vmovdqa32 %%zmm2, %0"                                              \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(src2), "r"(mask), "m"(dst_init)                    \
            : "zmm0", "zmm1", "zmm2", "k1");                                     \
    } while (0)

/*
 * Two operand form with an immediate: source and destination.  The byte
 * shifts do not accept a destination mask in this assembler.
 */
#define RUN_2OP_IMM(ins, imm, out)                                              \
    do {                                                                        \
        __asm__ volatile(                                                       \
            "vmovdqa32 %1, %%zmm0\n\t"                                          \
            "vmovdqa32 %2, %%zmm2\n\t"                                          \
            ins " $" imm ", %%zmm0, %%zmm2\n\t"                                 \
            "vmovdqa32 %%zmm2, %0"                                              \
            : "=m"(out)                                                         \
            : "m"(src1), "m"(dst_init)                                          \
            : "zmm0", "zmm2");                                                  \
    } while (0)

static void test_unpack(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /* The unpacks work inside every 128 bit lane. */
    RUN_3OP("vunpcklps", out);
    for (int i = 0; i < 16; i++) {
        int lane = i / 4, pos = i % 4;
        uint32_t expected = (pos & 1) ? src2.u32[lane * 4 + (pos >> 1)]
                                      : src1.u32[lane * 4 + (pos >> 1)];
        TEST_ASSERT(out.u32[i] == expected, "VUNPCKLPS lane %d", i);
    }

    RUN_3OP("vunpckhps", out);
    for (int i = 0; i < 16; i++) {
        int lane = i / 4, pos = i % 4;
        uint32_t expected = (pos & 1) ? src2.u32[lane * 4 + 2 + (pos >> 1)]
                                      : src1.u32[lane * 4 + 2 + (pos >> 1)];
        TEST_ASSERT(out.u32[i] == expected, "VUNPCKHPS lane %d", i);
    }

    RUN_3OP("vunpcklpd", out);
    for (int i = 0; i < 8; i++) {
        int lane = i / 2, pos = i % 2;
        uint64_t expected = (pos & 1) ? src2.u64[lane * 2] : src1.u64[lane * 2];
        TEST_ASSERT(out.u64[i] == expected, "VUNPCKLPD lane %d", i);
    }

    RUN_3OP("vunpckhpd", out);
    for (int i = 0; i < 8; i++) {
        int lane = i / 2, pos = i % 2;
        uint64_t expected = (pos & 1) ? src2.u64[lane * 2 + 1] : src1.u64[lane * 2 + 1];
        TEST_ASSERT(out.u64[i] == expected, "VUNPCKHPD lane %d", i);
    }
}

static int16_t saturate_i16(int32_t value) {
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static uint16_t saturate_u16(int32_t value) {
    if (value > UINT16_MAX) return UINT16_MAX;
    if (value < 0) return 0;
    return (uint16_t)value;
}

static void test_pack(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    /* VPACKSSDW saturates the signed dwords to words. */
    for (int i = 0; i < 16; i++) {
        src1.i32[i] = (i & 1) ? INT32_MAX : INT32_MIN;
        src2.i32[i] = (i & 1) ? INT32_MIN + 1 : 12345;
    }
    /*
     * The packed destination takes the even 128 bit lanes from the register
     * source and the odd lanes from the memory capable source.
     */
    RUN_3OP("vpackssdw", out);
    for (int lane = 0; lane < 4; lane++) {
        int base = (lane >> 1) * 4;
        for (int i = 0; i < 4; i++) {
            int32_t source = (lane & 1) ? src2.i32[base + i] : src1.i32[base + i];
            TEST_ASSERT(out.i16[lane * 4 + i] == saturate_i16(source),
                        "VPACKSSDW lane %d word %d", lane, i);
        }
    }

    /* VPACKUSDW saturates the signed dwords to unsigned words. */
    RUN_3OP("vpackusdw", out);
    for (int lane = 0; lane < 4; lane++) {
        int base = (lane >> 1) * 4;
        for (int i = 0; i < 4; i++) {
            int32_t source = (lane & 1) ? src2.i32[base + i] : src1.i32[base + i];
            TEST_ASSERT(out.u16[lane * 4 + i] == saturate_u16(source),
                        "VPACKUSDW lane %d word %d", lane, i);
        }
    }

    /* Values inside the destination range are copied unchanged. */
    for (int i = 0; i < 16; i++) {
        src1.i32[i] = i * 1000;
        src2.i32[i] = -i * 1000;
    }
    RUN_3OP("vpackssdw", out);
    for (int lane = 0; lane < 4; lane++) {
        int base = (lane >> 1) * 4;
        for (int i = 0; i < 4; i++) {
            int32_t source = (lane & 1) ? src2.i32[base + i] : src1.i32[base + i];
            TEST_ASSERT(out.i16[lane * 4 + i] == (int16_t)source,
                        "VPACKSSDW in range lane %d word %d", lane, i);
        }
    }
}

static void test_shift_bytes(void) {
    init_operands();

    /* VPSLLDQ shifts the bytes of every 128 bit lane towards the high end. */
    RUN_2OP_IMM("vpslldq", "4", out);
    for (int lane = 0; lane < 4; lane++) {
        for (int i = 0; i < 16; i++) {
            uint8_t expected = (i < 4) ? 0U : src1.u8[lane * 16 + i - 4];
            TEST_ASSERT(out.u8[lane * 16 + i] == expected,
                        "VPSLLDQ lane %d byte %d", lane, i);
        }
    }

    /* VPSRLDQ shifts the bytes of every 128 bit lane towards the low end. */
    RUN_2OP_IMM("vpsrldq", "4", out);
    for (int lane = 0; lane < 4; lane++) {
        for (int i = 0; i < 16; i++) {
            uint8_t expected = (i >= 12) ? 0U : src1.u8[lane * 16 + i + 4];
            TEST_ASSERT(out.u8[lane * 16 + i] == expected,
                        "VPSRLDQ lane %d byte %d", lane, i);
        }
    }

    /* A shift of zero keeps the bytes in place. */
    RUN_2OP_IMM("vpslldq", "0", out);
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(out.u8[i] == src1.u8[i], "VPSLLDQ zero shift byte %d", i);
}

static void test_bitwise(void) {
    uint32_t mask = 0xffffU;

    init_operands();

    RUN_3OP("vporq", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (src1.u64[i] | src2.u64[i]), "VPORQ lane %d", i);

    RUN_3OP("vpandq", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (src1.u64[i] & src2.u64[i]), "VPANDQ lane %d", i);

    RUN_3OP("vpandnq", out);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (~src1.u64[i] & src2.u64[i]), "VPANDNQ lane %d", i);
}

int main(void) {
    TEST_START("EVEX AVX-512 unpack, pack, shift and bitwise instructions");
    if (!CHECK_AVX512F()) return 0;
    test_unpack();
    test_pack();
    test_shift_bytes();
    test_bitwise();
    TEST_END();
}
