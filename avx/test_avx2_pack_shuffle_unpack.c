/*
 * TARGETS: VPACKSSWB VPACKUSWB VPALIGNR VPBLENDD VPBLENDW
 * VPSHUFD VPSHUFHW VPSHUFLW VPSADBW VPUNPCKHBW VPUNPCKHWD
 * VPUNPCKHDQ VPUNPCKHQDQ VPUNPCKLBW VPUNPCKLWD VPUNPCKLDQ
 * VPUNPCKLQDQ VPXOR
 */
#include "../common.h"
#include <limits.h>

static int8_t sat_i16_to_i8(int16_t value) {
    return value > INT8_MAX ? INT8_MAX : value < INT8_MIN ? INT8_MIN : (int8_t)value;
}

static uint8_t sat_i16_to_u8(int16_t value) {
    return value > UINT8_MAX ? UINT8_MAX : value < 0 ? 0 : (uint8_t)value;
}

static void test_pack(void) {
    const ymm_t a = { .i16 = {
        INT16_MIN, -129, -128, -127, -1, 0, 1, 127,
        128, 255, 256, INT16_MAX, -300, 42, -42, 1000
    } };
    const ymm_t b = { .i16 = {
        32767, 300, 128, 127, 126, -128, -129, -32768,
        -1, 0, 1, 254, 255, 256, 30000, -30000
    } };
    ymm_t out;

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpacksswb %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int lane = 0; lane < 2; lane++) {
        for (int i = 0; i < 8; i++) {
            TEST_ASSERT(out.i8[lane * 16 + i] == sat_i16_to_i8(a.i16[lane * 8 + i]),
                        "VPACKSSWB source A lane %d element %d", lane, i);
            TEST_ASSERT(out.i8[lane * 16 + 8 + i] == sat_i16_to_i8(b.i16[lane * 8 + i]),
                        "VPACKSSWB source B lane %d element %d", lane, i);
        }
    }

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpackuswb %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int lane = 0; lane < 2; lane++) {
        for (int i = 0; i < 8; i++) {
            TEST_ASSERT(out.u8[lane * 16 + i] == sat_i16_to_u8(a.i16[lane * 8 + i]),
                        "VPACKUSWB source A lane %d element %d", lane, i);
            TEST_ASSERT(out.u8[lane * 16 + 8 + i] == sat_i16_to_u8(b.i16[lane * 8 + i]),
                        "VPACKUSWB source B lane %d element %d", lane, i);
        }
    }
}

static void test_align_blend_shuffle(void) {
    ymm_t a, b, out;
    for (int i = 0; i < 32; i++) {
        a.u8[i] = (uint8_t)i;
        b.u8[i] = (uint8_t)(100 + i);
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpalignr $5, %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int lane = 0; lane < 2; lane++) {
        uint8_t concatenated[32];
        memcpy(concatenated, &b.u8[lane * 16], 16);
        memcpy(concatenated + 16, &a.u8[lane * 16], 16);
        for (int i = 0; i < 16; i++)
            TEST_ASSERT(out.u8[lane * 16 + i] == concatenated[i + 5],
                        "VPALIGNR lane %d byte %d", lane, i);
    }

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpblendd $0xa5, %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 8; i++) {
        uint32_t expected = (UINT32_C(0xa5) & (1u << i)) ? b.u32[i] : a.u32[i];
        TEST_ASSERT(out.u32[i] == expected, "VPBLENDD lane %d", i);
    }

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpblendw $0xa5, %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 16; i++) {
        uint16_t expected = (UINT32_C(0xa5) & (1u << (i & 7))) ? b.u16[i] : a.u16[i];
        TEST_ASSERT(out.u16[i] == expected, "VPBLENDW lane %d", i);
    }

    __asm__ volatile("vpshufd $0x1b, %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(a) : "ymm0");
    for (int lane = 0; lane < 2; lane++)
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(out.u32[lane * 4 + i] == a.u32[lane * 4 + 3 - i],
                        "VPSHUFD lane %d element %d", lane, i);

    __asm__ volatile("vpshufhw $0x1b, %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(a) : "ymm0");
    for (int lane = 0; lane < 2; lane++) {
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(out.u16[lane * 8 + i] == a.u16[lane * 8 + i],
                        "VPSHUFHW preserved lane %d element %d", lane, i);
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(out.u16[lane * 8 + 4 + i] == a.u16[lane * 8 + 7 - i],
                        "VPSHUFHW shuffled lane %d element %d", lane, i);
    }

    __asm__ volatile("vpshuflw $0x1b, %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(a) : "ymm0");
    for (int lane = 0; lane < 2; lane++) {
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(out.u16[lane * 8 + i] == a.u16[lane * 8 + 3 - i],
                        "VPSHUFLW shuffled lane %d element %d", lane, i);
        for (int i = 4; i < 8; i++)
            TEST_ASSERT(out.u16[lane * 8 + i] == a.u16[lane * 8 + i],
                        "VPSHUFLW preserved lane %d element %d", lane, i);
    }
}

static void test_sad(void) {
    ymm_t a, b, out;
    for (int i = 0; i < 32; i++) {
        a.u8[i] = (uint8_t)(i * 7);
        b.u8[i] = (uint8_t)(255 - i * 3);
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpsadbw %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int group = 0; group < 4; group++) {
        uint64_t expected = 0;
        for (int i = 0; i < 8; i++) {
            int av = a.u8[group * 8 + i];
            int bv = b.u8[group * 8 + i];
            expected += (uint64_t)(av > bv ? av - bv : bv - av);
        }
        TEST_ASSERT(out.u64[group] == expected, "VPSADBW group %d", group);
    }
}

#define RUN_UNPACK(NAME, INSN, FIELD, TYPE, PER_LANE, HIGH) do {             \
    ymm_t out;                                                               \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");  \
    for (int lane = 0; lane < 2; lane++) {                                  \
        int source = lane * (PER_LANE) + ((HIGH) ? (PER_LANE) / 2 : 0);     \
        int dest = lane * (PER_LANE);                                       \
        for (int i = 0; i < (PER_LANE) / 2; i++) {                          \
            TEST_ASSERT(out.FIELD[dest + 2 * i] == a.FIELD[source + i],     \
                        NAME " source A lane %d element %d", lane, i);     \
            TEST_ASSERT(out.FIELD[dest + 2 * i + 1] == b.FIELD[source + i], \
                        NAME " source B lane %d element %d", lane, i);     \
        }                                                                    \
    }                                                                        \
} while (0)

static void test_unpack_xor(void) {
    ymm_t a, b, out;
    for (int i = 0; i < 32; i++) {
        a.u8[i] = (uint8_t)i;
        b.u8[i] = (uint8_t)(0x80 + i);
    }
    RUN_UNPACK("VPUNPCKLBW", "vpunpcklbw", u8, uint8_t, 16, 0);
    RUN_UNPACK("VPUNPCKHBW", "vpunpckhbw", u8, uint8_t, 16, 1);
    RUN_UNPACK("VPUNPCKLWD", "vpunpcklwd", u16, uint16_t, 8, 0);
    RUN_UNPACK("VPUNPCKHWD", "vpunpckhwd", u16, uint16_t, 8, 1);
    RUN_UNPACK("VPUNPCKLDQ", "vpunpckldq", u32, uint32_t, 4, 0);
    RUN_UNPACK("VPUNPCKHDQ", "vpunpckhdq", u32, uint32_t, 4, 1);
    RUN_UNPACK("VPUNPCKLQDQ", "vpunpcklqdq", u64, uint64_t, 2, 0);
    RUN_UNPACK("VPUNPCKHQDQ", "vpunpckhqdq", u64, uint64_t, 2, 1);

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpxor %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == (a.u64[i] ^ b.u64[i]), "VPXOR qword %d", i);
}

int main(void) {
    TEST_START("AVX2 pack, shuffle, SAD, unpack, and XOR gaps");
    test_pack();
    test_align_blend_shuffle();
    test_sad();
    test_unpack_xor();
    __asm__ volatile("vzeroupper");
    TEST_END();
}
