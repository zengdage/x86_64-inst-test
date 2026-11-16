/*
 * TARGETS: VPADDB VPADDW VPADDD VPADDQ VPADDSB VPADDSW VPADDUSB VPADDUSW
 * VPSUBB VPSUBW VPSUBD VPSUBQ VPSUBSB VPSUBSW VPSUBUSB VPSUBUSW
 * VPABSB VPABSW VPABSD VPSIGNB VPSIGNW VPSIGND VPAVGB VPAVGW
 */
#include "../common.h"
#include <limits.h>

static int8_t sat_add_i8(int8_t a, int8_t b) {
    int value = (int)a + b;
    return value > INT8_MAX ? INT8_MAX : value < INT8_MIN ? INT8_MIN : (int8_t)value;
}

static int8_t sat_sub_i8(int8_t a, int8_t b) {
    int value = (int)a - b;
    return value > INT8_MAX ? INT8_MAX : value < INT8_MIN ? INT8_MIN : (int8_t)value;
}

static int16_t sat_add_i16(int16_t a, int16_t b) {
    int32_t value = (int32_t)a + b;
    return value > INT16_MAX ? INT16_MAX : value < INT16_MIN ? INT16_MIN : (int16_t)value;
}

static int16_t sat_sub_i16(int16_t a, int16_t b) {
    int32_t value = (int32_t)a - b;
    return value > INT16_MAX ? INT16_MAX : value < INT16_MIN ? INT16_MIN : (int16_t)value;
}

static uint8_t sat_add_u8(uint8_t a, uint8_t b) {
    unsigned value = (unsigned)a + b;
    return value > UINT8_MAX ? UINT8_MAX : (uint8_t)value;
}

static uint8_t sat_sub_u8(uint8_t a, uint8_t b) {
    return a < b ? 0 : (uint8_t)(a - b);
}

static uint16_t sat_add_u16(uint16_t a, uint16_t b) {
    uint32_t value = (uint32_t)a + b;
    return value > UINT16_MAX ? UINT16_MAX : (uint16_t)value;
}

static uint16_t sat_sub_u16(uint16_t a, uint16_t b) {
    return a < b ? 0 : (uint16_t)(a - b);
}

#define RUN_WRAP(NAME, INSN, FIELD, TYPE, LANES, OP) do {                    \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.FIELD[i] = (TYPE)(~(TYPE)0 - (TYPE)(i * 17));                     \
        b.FIELD[i] = (TYPE)(i * 29 + 1);                                    \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"      \
                     INSN " %%ymm1, %%ymm0, %%ymm2\n\t"                   \
                     "vmovdqu %%ymm2, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b)                        \
                     : "ymm0", "ymm1", "ymm2");                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        TYPE expected = (TYPE)(a.FIELD[i] OP b.FIELD[i]);                    \
        TEST_ASSERT(out.FIELD[i] == expected, NAME " lane %d", i);         \
    }                                                                       \
} while (0)

#define RUN_SAT_SIGNED(NAME, INSN, FIELD, TYPE, LANES, MINV, MAXV, FN) do {  \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.FIELD[i] = (i % 4 == 0) ? (MAXV) : (i % 4 == 1) ? (MINV) :       \
                     (i % 4 == 2) ? (TYPE)-1 : (TYPE)(i * 97);              \
        b.FIELD[i] = (i % 4 == 0) ? (TYPE)1 : (i % 4 == 1) ? (TYPE)-1 :    \
                     (i % 4 == 2) ? (MAXV) : (MINV);                        \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"      \
                     INSN " %%ymm1, %%ymm0, %%ymm2\n\t"                   \
                     "vmovdqu %%ymm2, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b)                        \
                     : "ymm0", "ymm1", "ymm2");                         \
    for (int i = 0; i < (LANES); i++)                                      \
        TEST_ASSERT(out.FIELD[i] == FN(a.FIELD[i], b.FIELD[i]),             \
                    NAME " lane %d", i);                                   \
} while (0)

#define RUN_SAT_UNSIGNED(NAME, INSN, FIELD, TYPE, LANES, MAXV, FN) do {      \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.FIELD[i] = (i % 4 == 0) ? (MAXV) : (i % 4 == 1) ? 0 :            \
                     (TYPE)(i * 101);                                       \
        b.FIELD[i] = (i % 4 == 0) ? 1 : (i % 4 == 1) ? (MAXV) :           \
                     (TYPE)(i * 59 + 3);                                    \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"      \
                     INSN " %%ymm1, %%ymm0, %%ymm2\n\t"                   \
                     "vmovdqu %%ymm2, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b)                        \
                     : "ymm0", "ymm1", "ymm2");                         \
    for (int i = 0; i < (LANES); i++)                                      \
        TEST_ASSERT(out.FIELD[i] == FN(a.FIELD[i], b.FIELD[i]),             \
                    NAME " lane %d", i);                                   \
} while (0)

#define RUN_AVG(NAME, INSN, FIELD, TYPE, WIDE, LANES) do {                  \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.FIELD[i] = (TYPE)(i * 37 + (i & 1));                              \
        b.FIELD[i] = (TYPE)(~a.FIELD[i] - (TYPE)(i & 3));                   \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");  \
    for (int i = 0; i < (LANES); i++) {                                     \
        TYPE expected = (TYPE)(((WIDE)a.FIELD[i] + b.FIELD[i] + 1) >> 1);   \
        TEST_ASSERT(out.FIELD[i] == expected, NAME " lane %d", i);         \
    }                                                                       \
} while (0)

#define RUN_ABS(NAME, INSN, SFIELD, UFIELD, STYPE, UTYPE, LANES, MINV, MAXV) do { \
    ymm_t a, out;                                                           \
    for (int i = 0; i < (LANES); i++)                                      \
        a.SFIELD[i] = (i == 0) ? (MINV) : (i == 1) ? (MAXV) :              \
                      (i & 1) ? (STYPE)-i : (STYPE)i;                       \
    __asm__ volatile(INSN " %1, %%ymm0\n\tvmovdqu %%ymm0, %0"            \
                     : "=m"(out) : "m"(a) : "ymm0");                    \
    for (int i = 0; i < (LANES); i++) {                                    \
        UTYPE value = (UTYPE)a.SFIELD[i];                                  \
        UTYPE expected = a.SFIELD[i] < 0 ? (UTYPE)(0 - value) : value;     \
        TEST_ASSERT(out.UFIELD[i] == expected, NAME " lane %d", i);       \
    }                                                                      \
} while (0)

#define RUN_SIGN(NAME, INSN, SFIELD, UFIELD, STYPE, UTYPE, LANES) do {      \
    ymm_t a, signs, out;                                                     \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.SFIELD[i] = (STYPE)(i * 13 + 1);                                  \
        signs.SFIELD[i] = (i % 3 == 0) ? (STYPE)-1 :                        \
                           (i % 3 == 1) ? (STYPE)0 : (STYPE)1;               \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(signs) : "ymm0", "ymm1"); \
    for (int i = 0; i < (LANES); i++) {                                     \
        UTYPE value = (UTYPE)a.SFIELD[i];                                   \
        UTYPE expected = signs.SFIELD[i] < 0 ? (UTYPE)(0 - value) :         \
                         signs.SFIELD[i] == 0 ? 0 : value;                  \
        TEST_ASSERT(out.UFIELD[i] == expected, NAME " lane %d", i);       \
    }                                                                       \
} while (0)

int main(void) {
    TEST_START("AVX2 packed add/subtract, absolute value, sign, and average");
    RUN_WRAP("VPADDB", "vpaddb", u8, uint8_t, 32, +);
    RUN_WRAP("VPADDW", "vpaddw", u16, uint16_t, 16, +);
    RUN_WRAP("VPADDD", "vpaddd", u32, uint32_t, 8, +);
    RUN_WRAP("VPADDQ", "vpaddq", u64, uint64_t, 4, +);
    RUN_WRAP("VPSUBB", "vpsubb", u8, uint8_t, 32, -);
    RUN_WRAP("VPSUBW", "vpsubw", u16, uint16_t, 16, -);
    RUN_WRAP("VPSUBD", "vpsubd", u32, uint32_t, 8, -);
    RUN_WRAP("VPSUBQ", "vpsubq", u64, uint64_t, 4, -);

    RUN_SAT_SIGNED("VPADDSB", "vpaddsb", i8, int8_t, 32, INT8_MIN, INT8_MAX, sat_add_i8);
    RUN_SAT_SIGNED("VPADDSW", "vpaddsw", i16, int16_t, 16, INT16_MIN, INT16_MAX, sat_add_i16);
    RUN_SAT_SIGNED("VPSUBSB", "vpsubsb", i8, int8_t, 32, INT8_MIN, INT8_MAX, sat_sub_i8);
    RUN_SAT_SIGNED("VPSUBSW", "vpsubsw", i16, int16_t, 16, INT16_MIN, INT16_MAX, sat_sub_i16);
    RUN_SAT_UNSIGNED("VPADDUSB", "vpaddusb", u8, uint8_t, 32, UINT8_MAX, sat_add_u8);
    RUN_SAT_UNSIGNED("VPADDUSW", "vpaddusw", u16, uint16_t, 16, UINT16_MAX, sat_add_u16);
    RUN_SAT_UNSIGNED("VPSUBUSB", "vpsubusb", u8, uint8_t, 32, UINT8_MAX, sat_sub_u8);
    RUN_SAT_UNSIGNED("VPSUBUSW", "vpsubusw", u16, uint16_t, 16, UINT16_MAX, sat_sub_u16);

    RUN_AVG("VPAVGB", "vpavgb", u8, uint8_t, uint16_t, 32);
    RUN_AVG("VPAVGW", "vpavgw", u16, uint16_t, uint32_t, 16);
    RUN_ABS("VPABSB", "vpabsb", i8, u8, int8_t, uint8_t, 32, INT8_MIN, INT8_MAX);
    RUN_ABS("VPABSW", "vpabsw", i16, u16, int16_t, uint16_t, 16, INT16_MIN, INT16_MAX);
    RUN_ABS("VPABSD", "vpabsd", i32, u32, int32_t, uint32_t, 8, INT32_MIN, INT32_MAX);
    RUN_SIGN("VPSIGNB", "vpsignb", i8, u8, int8_t, uint8_t, 32);
    RUN_SIGN("VPSIGNW", "vpsignw", i16, u16, int16_t, uint16_t, 16);
    RUN_SIGN("VPSIGND", "vpsignd", i32, u32, int32_t, uint32_t, 8);

    ymm_t initial, out;
    xmm_t low_a = { .u8 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16} };
    xmm_t low_b = { .u8 = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1} };
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm2\n\tvmovdqu %2, %%xmm0\n\t"
                     "vpaddb %3, %%xmm0, %%xmm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(initial), "m"(low_a), "m"(low_b)
                     : "xmm0", "ymm2");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u8[i] == (uint8_t)(low_a.u8[i] + low_b.u8[i]),
                    "VPADDB VEX.128 lane %d", i);
    for (int i = 16; i < 32; i++)
        TEST_ASSERT(out.u8[i] == 0, "VPADDB VEX.128 clears upper lane %d", i);
    __asm__ volatile("vzeroupper");
    TEST_END();
}
