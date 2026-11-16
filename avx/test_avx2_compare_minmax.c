/*
 * TARGETS: VPCMPEQB VPCMPEQW VPCMPEQD VPCMPEQQ
 * VPCMPGTB VPCMPGTW VPCMPGTD VPCMPGTQ
 * VPMAXSB VPMAXSW VPMAXUW VPMAXUD
 * VPMINSB VPMINSW VPMINUB VPMINUW VPMINUD
 */
#include "../common.h"
#include <limits.h>

#define RUN_EQ(NAME, INSN, FIELD, TYPE, LANES) do {                          \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.FIELD[i] = (TYPE)(i * 0x31u + 7u);                                \
        b.FIELD[i] = (i & 1) ? (TYPE)(a.FIELD[i] ^ (TYPE)1) : a.FIELD[i];   \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");  \
    for (int i = 0; i < (LANES); i++) {                                     \
        TYPE expected = a.FIELD[i] == b.FIELD[i] ? (TYPE)~(TYPE)0 : 0;      \
        TEST_ASSERT(out.FIELD[i] == expected, NAME " lane %d", i);         \
    }                                                                       \
} while (0)

#define RUN_GT(NAME, INSN, SFIELD, UFIELD, STYPE, UTYPE, LANES, MINV, MAXV) do { \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.SFIELD[i] = (i % 4 == 0) ? (MINV) : (i % 4 == 1) ? (MAXV) :      \
                      (i % 4 == 2) ? (STYPE)i : (STYPE)-i;                  \
        b.SFIELD[i] = (i % 4 == 0) ? (MAXV) : (i % 4 == 1) ? (MINV) :      \
                      (i % 4 == 2) ? (STYPE)i : (STYPE)(-i - 1);            \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");  \
    for (int i = 0; i < (LANES); i++) {                                     \
        UTYPE expected = a.SFIELD[i] > b.SFIELD[i] ? (UTYPE)~(UTYPE)0 : 0; \
        TEST_ASSERT(out.UFIELD[i] == expected, NAME " lane %d", i);        \
    }                                                                       \
} while (0)

#define RUN_SIGNED_MINMAX(NAME, INSN, FIELD, TYPE, LANES, MINV, MAXV, ISMAX) do { \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.FIELD[i] = (i % 4 == 0) ? (MINV) : (i % 4 == 1) ? (MAXV) :       \
                     (TYPE)(i * 43 - 300);                                  \
        b.FIELD[i] = (i % 4 == 0) ? (MAXV) : (i % 4 == 1) ? (MINV) :       \
                     (TYPE)(200 - i * 29);                                  \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");  \
    for (int i = 0; i < (LANES); i++) {                                     \
        TYPE expected = (ISMAX) ? (a.FIELD[i] > b.FIELD[i] ? a.FIELD[i] : b.FIELD[i]) \
                                : (a.FIELD[i] < b.FIELD[i] ? a.FIELD[i] : b.FIELD[i]); \
        TEST_ASSERT(out.FIELD[i] == expected, NAME " lane %d", i);         \
    }                                                                       \
} while (0)

#define RUN_UNSIGNED_MINMAX(NAME, INSN, FIELD, TYPE, LANES, MAXV, ISMAX) do { \
    ymm_t a, b, out;                                                         \
    for (int i = 0; i < (LANES); i++) {                                     \
        a.FIELD[i] = (i % 4 == 0) ? 0 : (i % 4 == 1) ? (MAXV) :            \
                     (TYPE)(i * 1009u + 17u);                               \
        b.FIELD[i] = (i % 4 == 0) ? (MAXV) : (i % 4 == 1) ? 0 :            \
                     (TYPE)((MAXV) - (TYPE)(i * 701u));                     \
    }                                                                       \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" INSN " %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1");  \
    for (int i = 0; i < (LANES); i++) {                                     \
        TYPE expected = (ISMAX) ? (a.FIELD[i] > b.FIELD[i] ? a.FIELD[i] : b.FIELD[i]) \
                                : (a.FIELD[i] < b.FIELD[i] ? a.FIELD[i] : b.FIELD[i]); \
        TEST_ASSERT(out.FIELD[i] == expected, NAME " lane %d", i);         \
    }                                                                       \
} while (0)

int main(void) {
    TEST_START("AVX2 packed compare and min/max gaps");
    RUN_EQ("VPCMPEQB", "vpcmpeqb", u8, uint8_t, 32);
    RUN_EQ("VPCMPEQW", "vpcmpeqw", u16, uint16_t, 16);
    RUN_EQ("VPCMPEQD", "vpcmpeqd", u32, uint32_t, 8);
    RUN_EQ("VPCMPEQQ", "vpcmpeqq", u64, uint64_t, 4);
    RUN_GT("VPCMPGTB", "vpcmpgtb", i8, u8, int8_t, uint8_t, 32, INT8_MIN, INT8_MAX);
    RUN_GT("VPCMPGTW", "vpcmpgtw", i16, u16, int16_t, uint16_t, 16, INT16_MIN, INT16_MAX);
    RUN_GT("VPCMPGTD", "vpcmpgtd", i32, u32, int32_t, uint32_t, 8, INT32_MIN, INT32_MAX);
    RUN_GT("VPCMPGTQ", "vpcmpgtq", i64, u64, int64_t, uint64_t, 4, INT64_MIN, INT64_MAX);

    RUN_SIGNED_MINMAX("VPMAXSB", "vpmaxsb", i8, int8_t, 32, INT8_MIN, INT8_MAX, 1);
    RUN_SIGNED_MINMAX("VPMAXSW", "vpmaxsw", i16, int16_t, 16, INT16_MIN, INT16_MAX, 1);
    RUN_UNSIGNED_MINMAX("VPMAXUW", "vpmaxuw", u16, uint16_t, 16, UINT16_MAX, 1);
    RUN_UNSIGNED_MINMAX("VPMAXUD", "vpmaxud", u32, uint32_t, 8, UINT32_MAX, 1);
    RUN_SIGNED_MINMAX("VPMINSB", "vpminsb", i8, int8_t, 32, INT8_MIN, INT8_MAX, 0);
    RUN_SIGNED_MINMAX("VPMINSW", "vpminsw", i16, int16_t, 16, INT16_MIN, INT16_MAX, 0);
    RUN_UNSIGNED_MINMAX("VPMINUB", "vpminub", u8, uint8_t, 32, UINT8_MAX, 0);
    RUN_UNSIGNED_MINMAX("VPMINUW", "vpminuw", u16, uint16_t, 16, UINT16_MAX, 0);
    RUN_UNSIGNED_MINMAX("VPMINUD", "vpminud", u32, uint32_t, 8, UINT32_MAX, 0);
    __asm__ volatile("vzeroupper");
    TEST_END();
}
