/*
 * test_phadd_phsub.c - Test PHADDW/PHADDD/PHADDSW/PHSUBW/PHSUBD/PHSUBSW (SSSE3)
 *
 * PHADDW:  Packed horizontal add words.
 * PHADDD:  Packed horizontal add dwords.
 * PHADDSW: Packed horizontal add words with saturation.
 * PHSUBW:  Packed horizontal subtract words.
 * PHSUBD:  Packed horizontal subtract dwords.
 * PHSUBSW: Packed horizontal subtract words with saturation.
 *
 * Horizontal operations add/subtract adjacent pairs within operands.
 *
 * Compile: gcc -o test_phadd_phsub simd/test_phadd_phsub.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_phaddw(void) {
    xmm_t a = { .i16 = {1, 2, 3, 4, 5, 6, 7, 8} };
    xmm_t b = { .i16 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Low 4: a[0]+a[1], a[2]+a[3], a[4]+a[5], a[6]+a[7] */
    TEST_ASSERT(dst.i16[0] == 3, "phaddw a[0]+a[1]: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 7, "phaddw a[2]+a[3]: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 11, "phaddw a[4]+a[5]: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 15, "phaddw a[6]+a[7]: got %d", dst.i16[3]);
    /* High 4: b[0]+b[1], b[2]+b[3], b[4]+b[5], b[6]+b[7] */
    TEST_ASSERT(dst.i16[4] == 30, "phaddw b[0]+b[1]: got %d", dst.i16[4]);
    TEST_ASSERT(dst.i16[5] == 70, "phaddw b[2]+b[3]: got %d", dst.i16[5]);
    TEST_ASSERT(dst.i16[6] == 110, "phaddw b[4]+b[5]: got %d", dst.i16[6]);
    TEST_ASSERT(dst.i16[7] == 150, "phaddw b[6]+b[7]: got %d", dst.i16[7]);
}

static void test_phaddd(void) {
    xmm_t a = { .i32 = {100, 200, 300, 400} };
    xmm_t b = { .i32 = {1000, 2000, 3000, 4000} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 300, "phaddd a[0]+a[1]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 700, "phaddd a[2]+a[3]: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 3000, "phaddd b[0]+b[1]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 7000, "phaddd b[2]+b[3]: got %d", dst.i32[3]);
}

static void test_phaddsw_saturation(void) {
    xmm_t a = { .i16 = {32767, 1, -32768, -1, 100, 200, -100, -200} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767, "phaddsw 32767+1 saturates: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "phaddsw -32768+-1 saturates: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 300, "phaddsw 100+200: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -300, "phaddsw -100+-200: got %d", dst.i16[3]);
}

static void test_phsubw(void) {
    xmm_t a = { .i16 = {10, 3, 20, 5, 30, 7, 40, 9} };
    xmm_t b = { .i16 = {100, 50, 200, 100, 300, 150, 400, 200} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* a[0]-a[1], a[2]-a[3], a[4]-a[5], a[6]-a[7] */
    TEST_ASSERT(dst.i16[0] == 7, "phsubw a[0]-a[1]: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 15, "phsubw a[2]-a[3]: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 23, "phsubw a[4]-a[5]: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 31, "phsubw a[6]-a[7]: got %d", dst.i16[3]);
    TEST_ASSERT(dst.i16[4] == 50, "phsubw b[0]-b[1]: got %d", dst.i16[4]);
}

static void test_phsubd(void) {
    xmm_t a = { .i32 = {1000, 300, 5000, 2000} };
    xmm_t b = { .i32 = {100, 100, 500, 200} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 700, "phsubd a[0]-a[1]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 3000, "phsubd a[2]-a[3]: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 0, "phsubd b[0]-b[1]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 300, "phsubd b[2]-b[3]: got %d", dst.i32[3]);
}

static void test_phsubsw_saturation(void) {
    xmm_t a = { .i16 = {32767, -1, -32768, 1, 0,0,0,0} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 32767 - (-1) = 32768, saturates to 32767 */
    TEST_ASSERT(dst.i16[0] == 32767, "phsubsw 32767-(-1) saturates: got %d", dst.i16[0]);
    /* -32768 - 1 = -32769, saturates to -32768 */
    TEST_ASSERT(dst.i16[1] == -32768, "phsubsw -32768-1 saturates: got %d", dst.i16[1]);
}

/* ----- Boundary / edge-case tests ----- */

/* PHADDW: all-zero operands. */
static void test_phaddw_zeros(void) {
    xmm_t a = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (i = 0; i < 8; i++) {
        TEST_ASSERT(dst.i16[i] == 0, "phaddw zeros[%d]: got %d", i, dst.i16[i]);
    }
}

/* PHADDW: identical operands (a == b). Verifies the destination is fully
 * overwritten — the result should be a horizontal-add of a alone, repeated. */
static void test_phaddw_same_operand(void) {
    xmm_t a = { .i16 = {1, 2, 3, 4, 5, 6, 7, 8} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddw %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 3,  "phaddw a==b [0]: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 7,  "phaddw a==b [1]: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 11, "phaddw a==b [2]: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 15, "phaddw a==b [3]: got %d", dst.i16[3]);
    /* high quadword is also a's horizontal sum */
    TEST_ASSERT(dst.i16[4] == 3,  "phaddw a==b [4]: got %d", dst.i16[4]);
    TEST_ASSERT(dst.i16[5] == 7,  "phaddw a==b [5]: got %d", dst.i16[5]);
    TEST_ASSERT(dst.i16[6] == 11, "phaddw a==b [6]: got %d", dst.i16[6]);
    TEST_ASSERT(dst.i16[7] == 15, "phaddw a==b [7]: got %d", dst.i16[7]);
}

/* PHADDW: negative operands. */
static void test_phaddw_negative(void) {
    xmm_t a = { .i16 = {-1, -2, -3, -4, -100, -200, -1000, -2000} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == -3,   "phaddw -1+-2: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -7,   "phaddw -3+-4: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == -300, "phaddw -100+-200: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -3000,"phaddw -1000+-2000: got %d", dst.i16[3]);
}

/* PHADDW: 16-bit signed wrap-around at the boundary. PHADDW does NOT
 * saturate, so the result is the low 16 bits of the true sum. */
static void test_phaddw_overflow(void) {
    /* 0x7FFF + 0x0001 = 0x8000 (i16: -32768) */
    /* 0x8000 + 0x8000 = 0x10000 -> low16 = 0x0000 */
    /* 0x7FFF + 0x7FFF = 0xFFFE (i16: -2) */
    xmm_t a = { .i16 = {0x7FFF, 0x0001, 0x8000, 0x8000, 0x7FFF, 0x7FFF, 0, 0} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == (int16_t)0x8000, "phaddw 0x7FFF+0x0001 wraps: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 0,                 "phaddw 0x8000+0x8000 wraps: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == -2,                "phaddw 0x7FFF+0x7FFF wraps: got %d", dst.i16[2]);
}

/* PHADDD: zero operands and identical-operand self-add. */
static void test_phaddd_zeros(void) {
    xmm_t a = { .i32 = {0,0,0,0} };
    xmm_t b = { .i32 = {0,0,0,0} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (i = 0; i < 4; i++) {
        TEST_ASSERT(dst.i32[i] == 0, "phaddd zeros[%d]: got %d", i, dst.i32[i]);
    }
}

static void test_phaddd_same_operand(void) {
    xmm_t a = { .i32 = {10, 20, 30, 40} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 30,  "phaddd a==b [0]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 70,  "phaddd a==b [1]: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 30,  "phaddd a==b [2]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 70,  "phaddd a==b [3]: got %d", dst.i32[3]);
}

/* PHADDD: 32-bit signed wrap-around at INT32_MAX / INT32_MIN boundary. */
static void test_phaddd_overflow(void) {
    /* INT32_MAX + 1               wraps to INT32_MIN (0x80000000) */
    /* INT32_MAX + INT32_MAX       wraps to -2 (0xFFFFFFFE) */
    /* INT32_MIN + INT32_MIN       wraps to 0  (0x100000000 -> 0) */
    xmm_t a = { .i32 = {0x7FFFFFFF, 0x00000001, 0x7FFFFFFF, 0x7FFFFFFF} };
    xmm_t b = { .i32 = {0x80000000, 0x80000000, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == (int32_t)0x80000000, "phaddd MAX+1 wraps: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == -2,                  "phaddd MAX+MAX wraps: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 0,                   "phaddd MIN+MIN wraps: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 0,                   "phaddd high MIN+MIN wraps: got %d", dst.i32[3]);
}

/* PHADDSW: results that should NOT saturate match a regular PHADDW.
 * Boundary values: 0+32767, -32768+0, 1+32766, etc. */
static void test_phaddsw_no_saturation(void) {
    /* pairs: (0,32767)->32767, (-32768,0)->-32768, (1,32766)->32767,
     *        (-1,-32767)->-32768 */
    xmm_t a = { .i16 = {0, 32767, -32768, 0, 1, 32766, -1, -32767} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767,  "phaddsw 0+32767 no-sat: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "phaddsw -32768+0 no-sat: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 32767,  "phaddsw 1+32766 no-sat: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -32768, "phaddsw -1+-32767 no-sat: got %d", dst.i16[3]);
}

/* PHADDSW: zero operands, both identical. */
static void test_phaddsw_zeros(void) {
    xmm_t a = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddsw %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    for (i = 0; i < 8; i++) {
        TEST_ASSERT(dst.i16[i] == 0, "phaddsw zeros[%d]: got %d", i, dst.i16[i]);
    }
}

/* PHADDSW: just-past-the-edge saturations. */
static void test_phaddsw_sat_edges(void) {
    /* 0x7FFF + 0x0002 = 0x8001  ->  saturates to  32767 */
    /* (-0x8000) + (-0x8000) = -0x10000 -> saturates to -32768 */
    /* 0x4000 + 0x4001 = 0x8001  ->  saturates to  32767 */
    /* 0x7FFF + 0x0001 = 0x8000  ->  saturates to  32767 */
    xmm_t a = { .i16 = {0x7FFF, 0x0002, (int16_t)0x8000, (int16_t)0x8000,
                        0x4000, 0x4001, 0x7FFF, 0x0001} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767,  "phaddsw MAX+2 sat-up: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "phaddsw MIN+MIN sat-down: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 32767,  "phaddsw 0x4000+0x4001 sat-up: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 32767,  "phaddsw MAX+1 sat-up: got %d", dst.i16[3]);
}

/* PHSUBW: zero operands. */
static void test_phsubw_zeros(void) {
    xmm_t a = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (i = 0; i < 8; i++) {
        TEST_ASSERT(dst.i16[i] == 0, "phsubw zeros[%d]: got %d", i, dst.i16[i]);
    }
}

/* PHSUBW: identical operands → result is all zero. */
static void test_phsubw_same_operand(void) {
    xmm_t a = { .i16 = {1, 1, -2, -2, 32767, 32767, 0, 0} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubw %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    for (i = 0; i < 8; i++) {
        TEST_ASSERT(dst.i16[i] == 0, "phsubw a==b[%d]: got %d", i, dst.i16[i]);
    }
}

/* PHSUBW: negative-minuend. */
static void test_phsubw_negative(void) {
    /* a: (-5)-(-1) = -4,  (-3)-(-2) = -1,  (-100)-(-50) = -50,  (-9)-(-1) = -8 */
    xmm_t a = { .i16 = {-5, -1, -3, -2, -100, -50, -9, -1} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == -4,  "phsubw -5-(-1): got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -1,  "phsubw -3-(-2): got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == -50, "phsubw -100-(-50): got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -8,  "phsubw -9-(-1): got %d", dst.i16[3]);
}

/* PHSUBW: 16-bit signed wrap-around (no saturation). */
static void test_phsubw_overflow(void) {
    /* 0x7FFF - 0x8000 = 0xFFFF (i16: -1, no sat) */
    /* 0x8000 - 0x0001 = 0x7FFF (i16: 32767) */
    /* 0x0000 - 0x8000 = 0x8000 (i16: -32768) */
    /* 0x7FFF - 0xFFFF(=-1) = 0x8000 (i16: -32768) */
    xmm_t a = { .i16 = {0x7FFF, (int16_t)0x8000, 0x8000, 0x0001,
                        0x0000, (int16_t)0x8000, 0x7FFF, (int16_t)0xFFFF} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == (int16_t)0xFFFF, "phsubw 0x7FFF-0x8000 wraps: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 32767,           "phsubw 0x8000-0x0001 wraps: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == (int16_t)0x8000, "phsubw 0x0000-0x8000 wraps: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == (int16_t)0x8000, "phsubw 0x7FFF-(-1) wraps: got %d", dst.i16[3]);
}

/* PHSUBD: zero operands. */
static void test_phsubd_zeros(void) {
    xmm_t a = { .i32 = {0,0,0,0} };
    xmm_t b = { .i32 = {0,0,0,0} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (i = 0; i < 4; i++) {
        TEST_ASSERT(dst.i32[i] == 0, "phsubd zeros[%d]: got %d", i, dst.i32[i]);
    }
}

/* PHSUBD: identical operands → all zero. */
static void test_phsubd_same_operand(void) {
    xmm_t a = { .i32 = {42, 42, -7, -7} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubd %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    for (i = 0; i < 4; i++) {
        TEST_ASSERT(dst.i32[i] == 0, "phsubd a==b[%d]: got %d", i, dst.i32[i]);
    }
}

/* PHSUBD: 32-bit signed wrap-around at INT32 boundaries. */
static void test_phsubd_overflow(void) {
    /* dst[0] = a[0]-a[1] = 0x7FFFFFFF - 0x80000000 = MAX - MIN -> 0xFFFFFFFF (i32: -1) */
    /* dst[1] = a[2]-a[3] = 0x80000000 - 0x00000001 = MIN - 1 -> 0x7FFFFFFF (i32: INT32_MAX) */
    /* dst[2] = b[0]-b[1] = 0x00000000 - 0x7FFFFFFF = 0 - MAX -> 0x80000001 (i32: -INT32_MAX) */
    /* dst[3] = b[2]-b[3] = 0x7FFFFFFF - 0xFFFFFFFF(-1) = MAX+1 -> 0x80000000 (i32: INT32_MIN) */
    xmm_t a = { .i32 = {0x7FFFFFFF, 0x80000000, 0x80000000, 0x00000001} };
    xmm_t b = { .i32 = {0x00000000, 0x7FFFFFFF, 0x7FFFFFFF, (int32_t)0xFFFFFFFF} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == -1,                 "phsubd MAX-MIN wraps: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 0x7FFFFFFF,         "phsubd MIN-1 wraps: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == (int32_t)0x80000001,"phsubd 0-MAX wraps: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == (int32_t)0x80000000,"phsubd MAX-(-1) wraps: got %d", dst.i32[3]);
}

/* PHSUBSW: results that should NOT saturate. Verify equivalence with PHSUBW. */
static void test_phsubsw_no_saturation(void) {
    /* 32767 - 0      = 32767 */
    /* 0    - 32767   = -32767 */
    /* -32768 - 0     = -32768 */
    /* -1    - 32767  = -32768 */
    xmm_t a = { .i16 = {32767, 0, 0, 32767, -32768, 0, -1, 32767} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767,  "phsubsw 32767-0 no-sat: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32767, "phsubsw 0-32767 no-sat: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == -32768, "phsubsw -32768-0 no-sat: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -32768, "phsubsw -1-32767 no-sat: got %d", dst.i16[3]);
}

/* PHSUBSW: zero operands (both a and b zero). */
static void test_phsubsw_zeros(void) {
    xmm_t a = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;
    int i;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (i = 0; i < 8; i++) {
        TEST_ASSERT(dst.i16[i] == 0, "phsubsw zeros[%d]: got %d", i, dst.i16[i]);
    }
}

/* PHSUBSW: just-past-the-edge saturations. */
static void test_phsubsw_sat_edges(void) {
    /* dst[0] = a[0]-a[1] = 0x7FFF - 0xFFFF(-1) = MAX+1 -> saturates to  32767 */
    /* dst[1] = a[2]-a[3] = 0x8000 - 0x7FFF = MIN-MAX = -65535 -> saturates to -32768 */
    /* dst[2] = a[4]-a[5] = 0x7FFF - 0x8000 = MAX-MIN = 65535 -> saturates to  32767 */
    /* dst[3] = a[6]-a[7] = 0x0000 - 0x7FFF = -MAX (i16: -32767, NO sat) */
    xmm_t a = { .i16 = {0x7FFF, (int16_t)0xFFFF, (int16_t)0x8000, 0x7FFF,
                        0x7FFF, (int16_t)0x8000, 0x0000, 0x7FFF} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767,  "phsubsw MAX-(-1) sat-up: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "phsubsw MIN-MAX sat-down: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 32767,  "phsubsw MAX-MIN sat-up: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -32767, "phsubsw 0-MAX no-sat: got %d", dst.i16[3]);
}

/* PHADDW/PHADDD: both operands non-zero (exercises the high 64/128 bits). */
static void test_phaddw_b_nonzero(void) {
    /* a: small positives, b: large positives */
    xmm_t a = { .i16 = {1, 2, 3, 4, 5, 6, 7, 8} };
    xmm_t b = { .i16 = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* low quad (from a) already covered; verify high quad is from b */
    TEST_ASSERT(dst.i16[4] == 3000,  "phaddw b[0]+b[1]: got %d", dst.i16[4]);
    TEST_ASSERT(dst.i16[5] == 7000,  "phaddw b[2]+b[3]: got %d", dst.i16[5]);
    TEST_ASSERT(dst.i16[6] == 11000, "phaddw b[4]+b[5]: got %d", dst.i16[6]);
    TEST_ASSERT(dst.i16[7] == 15000, "phaddw b[6]+b[7]: got %d", dst.i16[7]);
}

static void test_phsubw_b_nonzero(void) {
    xmm_t a = { .i16 = {10, 3, 20, 5, 30, 7, 40, 9} };
    xmm_t b = { .i16 = {1000, 1, 2000, 2, 3000, 3, 4000, 4} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[4] == 999,   "phsubw b[0]-b[1]: got %d", dst.i16[4]);
    TEST_ASSERT(dst.i16[5] == 1998,  "phsubw b[2]-b[3]: got %d", dst.i16[5]);
    TEST_ASSERT(dst.i16[6] == 2997,  "phsubw b[4]-b[5]: got %d", dst.i16[6]);
    TEST_ASSERT(dst.i16[7] == 3996,  "phsubw b[6]-b[7]: got %d", dst.i16[7]);
}

static void test_phsubd_b_nonzero(void) {
    /* exercises high dword pair (b[2]-b[3]) with both operands non-zero */
    xmm_t a = { .i32 = {10, 3, 20, 5} };
    xmm_t b = { .i32 = {100, 1, 200, 2} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[2] == 99,  "phsubd b[0]-b[1]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 198, "phsubd b[2]-b[3]: got %d", dst.i32[3]);
}

int main(void) {
    TEST_START("PHADDW/PHADDD/PHADDSW/PHSUBW/PHSUBD/PHSUBSW instructions (SSSE3)");
    /* Original tests */
    test_phaddw();
    test_phaddd();
    test_phaddsw_saturation();
    test_phsubw();
    test_phsubd();
    test_phsubsw_saturation();
    /* Boundary / edge-case tests */
    test_phaddw_zeros();
    test_phaddw_same_operand();
    test_phaddw_negative();
    test_phaddw_overflow();
    test_phaddd_zeros();
    test_phaddd_same_operand();
    test_phaddd_overflow();
    test_phaddsw_zeros();
    test_phaddsw_no_saturation();
    test_phaddsw_sat_edges();
    test_phsubw_zeros();
    test_phsubw_same_operand();
    test_phsubw_negative();
    test_phsubw_overflow();
    test_phsubd_zeros();
    test_phsubd_same_operand();
    test_phsubd_overflow();
    test_phsubsw_zeros();
    test_phsubsw_no_saturation();
    test_phsubsw_sat_edges();
    test_phaddw_b_nonzero();
    test_phsubw_b_nonzero();
    test_phsubd_b_nonzero();
    TEST_END();
}
