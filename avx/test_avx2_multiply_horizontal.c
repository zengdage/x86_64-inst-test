/*
 * TARGETS: VPMADDWD VPMULDQ VPMULHRSW VPMULHUW VPMULHW VPMULLW
 * VPHADDW VPHADDD VPHADDSW VPHSUBW VPHSUBD VPHSUBSW
 */
#include "../common.h"
#include <limits.h>

static int16_t sat_i32_to_i16(int32_t value) {
    return value > INT16_MAX ? INT16_MAX :
           value < INT16_MIN ? INT16_MIN : (int16_t)value;
}

static int16_t mulhrsw(int16_t a, int16_t b) {
    int32_t value = ((int32_t)a * b + INT32_C(0x4000)) >> 15;
    return (int16_t)value;
}

static void test_multiply(void) {
    const ymm_t a = { .i16 = {
        INT16_MIN, INT16_MIN, INT16_MAX, 2, -30000, 12345, -1, 0,
        1, -2, 32700, -32700, 0x4000, -0x4000, 77, -91
    } };
    const ymm_t b = { .i16 = {
        INT16_MIN, INT16_MIN, 2, INT16_MAX, -2, -23456, INT16_MIN, INT16_MAX,
        -1, 3, -32700, 32700, 0x4000, 0x4000, -101, 113
    } };
    ymm_t out;

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmaddwd %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 8; i++) {
        int64_t sum = (int64_t)a.i16[2 * i] * b.i16[2 * i] +
                      (int64_t)a.i16[2 * i + 1] * b.i16[2 * i + 1];
        TEST_ASSERT(out.u32[i] == (uint32_t)sum, "VPMADDWD lane %d", i);
    }

    const ymm_t da = { .i32 = {
        INT32_MIN, 111, INT32_MAX, 222, -1, 333, 0x40000000, 444
    } };
    const ymm_t db = { .i32 = {-1, -9, 2, -8, INT32_MIN, -7, -8, -6} };
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmuldq %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(da), "m"(db) : "ymm0", "ymm1");
    for (int i = 0; i < 4; i++) {
        int index = i * 2;
        int64_t expected = (int64_t)da.i32[index] * db.i32[index];
        TEST_ASSERT(out.i64[i] == expected, "VPMULDQ lane %d", i);
    }

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmulhrsw %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.i16[i] == mulhrsw(a.i16[i], b.i16[i]),
                    "VPMULHRSW lane %d", i);

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmulhuw %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 16; i++) {
        uint16_t expected = (uint16_t)(((uint32_t)a.u16[i] * b.u16[i]) >> 16);
        TEST_ASSERT(out.u16[i] == expected, "VPMULHUW lane %d", i);
    }

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmulhw %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 16; i++) {
        int16_t expected = (int16_t)(((int32_t)a.i16[i] * b.i16[i]) >> 16);
        TEST_ASSERT(out.i16[i] == expected, "VPMULHW lane %d", i);
    }

    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvpmullw %2, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");
    for (int i = 0; i < 16; i++) {
        uint16_t expected = (uint16_t)((uint32_t)a.u16[i] * b.u16[i]);
        TEST_ASSERT(out.u16[i] == expected, "VPMULLW lane %d", i);
    }
}

#define RUN_HORIZONTAL_WORD(NAME, INSN, OP, SATURATE) do {                  \
    ymm_t out;                                                               \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");  \
    for (int lane = 0; lane < 2; lane++) {                                  \
        int base = lane * 8;                                                 \
        for (int pair = 0; pair < 4; pair++) {                              \
            int32_t av = (int32_t)a.i16[base + 2 * pair] OP                 \
                         a.i16[base + 2 * pair + 1];                         \
            int32_t bv = (int32_t)b.i16[base + 2 * pair] OP                 \
                         b.i16[base + 2 * pair + 1];                         \
            int16_t ae = (SATURATE) ? sat_i32_to_i16(av) : (int16_t)av;     \
            int16_t be = (SATURATE) ? sat_i32_to_i16(bv) : (int16_t)bv;     \
            TEST_ASSERT(out.i16[base + pair] == ae,                         \
                        NAME " source A lane %d pair %d", lane, pair);     \
            TEST_ASSERT(out.i16[base + 4 + pair] == be,                     \
                        NAME " source B lane %d pair %d", lane, pair);     \
        }                                                                    \
    }                                                                        \
} while (0)

#define RUN_HORIZONTAL_DWORD(NAME, INSN, OP) do {                           \
    ymm_t out;                                                               \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(da), "m"(db) : "ymm0", "ymm1"); \
    for (int lane = 0; lane < 2; lane++) {                                  \
        int base = lane * 4;                                                 \
        for (int pair = 0; pair < 2; pair++) {                              \
            uint32_t ae = da.u32[base + 2 * pair] OP da.u32[base + 2 * pair + 1]; \
            uint32_t be = db.u32[base + 2 * pair] OP db.u32[base + 2 * pair + 1]; \
            TEST_ASSERT(out.u32[base + pair] == ae,                         \
                        NAME " source A lane %d pair %d", lane, pair);     \
            TEST_ASSERT(out.u32[base + 2 + pair] == be,                     \
                        NAME " source B lane %d pair %d", lane, pair);     \
        }                                                                    \
    }                                                                        \
} while (0)

static void test_horizontal(void) {
    const ymm_t a = { .i16 = {
        INT16_MAX, 1, INT16_MIN, -1, 30000, 30000, -30000, -30000,
        100, -50, -200, 70, 1234, 5678, -1234, -5678
    } };
    const ymm_t b = { .i16 = {
        1, INT16_MAX, -1, INT16_MIN, 20000, -20000, -20000, 20000,
        -100, 50, 200, -70, -2222, 1111, 2222, -1111
    } };
    const ymm_t da = { .u32 = {
        0xffffffff, 1, 0x80000000, 0x80000000,
        100, 200, 0xfffffff0, 0x20
    } };
    const ymm_t db = { .u32 = {
        7, 9, 0, 1, 0x12345678, 0x11111111, 40, 100
    } };

    RUN_HORIZONTAL_WORD("VPHADDW", "vphaddw", +, 0);
    RUN_HORIZONTAL_WORD("VPHADDSW", "vphaddsw", +, 1);
    RUN_HORIZONTAL_WORD("VPHSUBW", "vphsubw", -, 0);
    RUN_HORIZONTAL_WORD("VPHSUBSW", "vphsubsw", -, 1);
    RUN_HORIZONTAL_DWORD("VPHADDD", "vphaddd", +);
    RUN_HORIZONTAL_DWORD("VPHSUBD", "vphsubd", -);
}

int main(void) {
    TEST_START("AVX2 multiply and horizontal arithmetic gaps");
    test_multiply();
    test_horizontal();
    __asm__ volatile("vzeroupper");
    TEST_END();
}
