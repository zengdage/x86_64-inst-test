/*
 * test_fild_fist.c - Test x87 FILD/FIST/FISTP/FISTTP instructions
 *
 * FILD: Load integer from memory and push as float onto FPU stack.
 * FIST: Store ST(0) as integer to memory (without popping), uses current rounding mode.
 * FISTP: Store ST(0) as integer and pop.
 * FISTTP: Store ST(0) as integer and pop, always truncates (SSE3).
 * Supports 16-bit, 32-bit, and 64-bit integer operands.
 *
 * Compile: gcc -o test_fild_fist float/test_fild_fist.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>

static void test_fild_16(void) {
    int16_t src;
    double result;

    src = 42;
    __asm__ volatile (
        "filds %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == 42.0, "fild 16-bit 42: got %f", result);

    src = -32768;
    __asm__ volatile (
        "filds %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == -32768.0, "fild 16-bit INT16_MIN: got %f", result);

    src = 32767;
    __asm__ volatile (
        "filds %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == 32767.0, "fild 16-bit INT16_MAX: got %f", result);

    src = 0;
    __asm__ volatile (
        "filds %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == 0.0, "fild 16-bit 0: got %f", result);
}

static void test_fild_32(void) {
    int32_t src;
    double result;

    src = 123456;
    __asm__ volatile (
        "fildl %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == 123456.0, "fild 32-bit 123456: got %f", result);

    src = -2147483647 - 1;  /* INT32_MIN */
    __asm__ volatile (
        "fildl %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == -2147483648.0, "fild 32-bit INT32_MIN: got %f", result);

    src = 2147483647;  /* INT32_MAX */
    __asm__ volatile (
        "fildl %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == 2147483647.0, "fild 32-bit INT32_MAX: got %f", result);
}

static void test_fild_64(void) {
    int64_t src;
    double result;

    src = 1000000000000LL;
    __asm__ volatile (
        "fildq %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == 1000000000000.0, "fild 64-bit 1e12: got %f", result);

    src = 0;
    __asm__ volatile (
        "fildq %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == 0.0, "fild 64-bit 0: got %f", result);

    src = -1;
    __asm__ volatile (
        "fildq %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(src)
    );
    TEST_ASSERT(result == -1.0, "fild 64-bit -1: got %f", result);
}

static void test_fist_16(void) {
    double src;
    int16_t dst;

    src = 42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fists %0"
        : "=m"(dst)
        : "m"(src)
    );
    /* Clean FPU stack */
    __asm__ volatile ("fstp %%st(0)" ::: "st");
    TEST_ASSERT(dst == 42, "fist 16-bit 42.0: got %d", dst);

    /* Rounds to nearest even by default */
    src = 2.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "fists %0"
        : "=m"(dst)
        : "m"(src)
    );
    __asm__ volatile ("fstp %%st(0)" ::: "st");
    TEST_ASSERT(dst == 2, "fist 16-bit 2.5 (banker's round): got %d", dst);

    src = 3.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "fists %0"
        : "=m"(dst)
        : "m"(src)
    );
    __asm__ volatile ("fstp %%st(0)" ::: "st");
    TEST_ASSERT(dst == 4, "fist 16-bit 3.5 (banker's round): got %d", dst);
}

static void test_fistp_32(void) {
    double src;
    int32_t dst;

    src = -999.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fistpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == -999, "fistp 32-bit -999: got %d", dst);

    src = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fistpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 0, "fistp 32-bit 0: got %d", dst);

    src = 7.7;
    __asm__ volatile (
        "fldl %1\n\t"
        "fistpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 8, "fistp 32-bit 7.7 (round to nearest): got %d", dst);
}

static void test_fistp_64(void) {
    double src;
    int64_t dst;

    src = 1000000000000.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fistpq %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 1000000000000LL, "fistp 64-bit 1e12: got %ld", dst);
}

static void test_fisttp(void) {
    double src;
    int32_t dst32;
    int16_t dst16;

    /* FISTTP always truncates (round toward zero) */
    src = 7.9;
    __asm__ volatile (
        "fldl %1\n\t"
        "fisttpl %0"
        : "=m"(dst32)
        : "m"(src)
    );
    TEST_ASSERT(dst32 == 7, "fisttp 32-bit 7.9 (truncate): got %d", dst32);

    src = -7.9;
    __asm__ volatile (
        "fldl %1\n\t"
        "fisttpl %0"
        : "=m"(dst32)
        : "m"(src)
    );
    TEST_ASSERT(dst32 == -7, "fisttp 32-bit -7.9 (truncate): got %d", dst32);

    src = 2.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "fisttps %0"
        : "=m"(dst16)
        : "m"(src)
    );
    TEST_ASSERT(dst16 == 2, "fisttp 16-bit 2.5 (truncate): got %d", dst16);

    src = -2.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "fisttps %0"
        : "=m"(dst16)
        : "m"(src)
    );
    TEST_ASSERT(dst16 == -2, "fisttp 16-bit -2.5 (truncate): got %d", dst16);

    /* FISTTP 64-bit */
    int64_t dst64;
    src = 999999999.9;
    __asm__ volatile (
        "fldl %1\n\t"
        "fisttpq %0"
        : "=m"(dst64)
        : "m"(src)
    );
    TEST_ASSERT(dst64 == 999999999LL, "fisttp 64-bit 999999999.9: got %ld", dst64);
}

static void test_fild_fist_boundaries(void) {
    int64_t src64;
    long double extended;
    double src;
    int32_t dst32;
    uint16_t status;

    /* 80-bit x87 precision represents every signed 64-bit integer exactly. */
    src64 = INT64_MIN;
    __asm__ volatile (
        "fildq %1\n\t"
        "fstpt %0"
        : "=m"(extended)
        : "m"(src64)
    );
    TEST_ASSERT(extended == (long double)INT64_MIN, "fildq INT64_MIN exact");

    src64 = INT64_MAX;
    __asm__ volatile (
        "fildq %1\n\t"
        "fstpt %0"
        : "=m"(extended)
        : "m"(src64)
    );
    TEST_ASSERT(extended == (long double)INT64_MAX, "fildq INT64_MAX exact");

    /* Masked invalid conversions store the integer-indefinite value and set IE. */
    src = INFINITY;
    __asm__ volatile (
        "fnclex\n\t"
        "fldl %2\n\t"
        "fistpl %0\n\t"
        "fnstsw %1"
        : "=m"(dst32), "=m"(status)
        : "m"(src)
        : "memory"
    );
    TEST_ASSERT((uint32_t)dst32 == UINT32_C(0x80000000),
                "fistp +inf: integer-indefinite result");
    TEST_ASSERT(status & 1, "fistp +inf: invalid-operation status set");

    src = NAN;
    __asm__ volatile (
        "fnclex\n\t"
        "fldl %2\n\t"
        "fisttpl %0\n\t"
        "fnstsw %1"
        : "=m"(dst32), "=m"(status)
        : "m"(src)
        : "memory"
    );
    TEST_ASSERT((uint32_t)dst32 == UINT32_C(0x80000000),
                "fisttp NaN: integer-indefinite result");
    TEST_ASSERT(status & 1, "fisttp NaN: invalid-operation status set");
}

static void test_fist_rounding_modes_widths_and_stack(void) {
    uint16_t saved_cw, cw;
    double positive = 2.5, negative = -2.5;
    int32_t pos_result, neg_result;
    const int32_t expected[4][2] = {
        {2, -2}, {2, -3}, {3, -2}, {2, -2}
    };
    __asm__ volatile("fnstcw %0" : "=m"(saved_cw));
    for (uint16_t mode = 0; mode < 4; mode++) {
        cw = (uint16_t)((saved_cw & ~UINT16_C(0x0c00)) | (mode << 10));
        __asm__ volatile("fldcw %0" : : "m"(cw));
        __asm__ volatile(
            "fldl %2\n\tfistpl %0\n\t"
            "fldl %3\n\tfistpl %1"
            : "=m"(pos_result), "=m"(neg_result)
            : "m"(positive), "m"(negative)
        );
        TEST_ASSERT(pos_result == expected[mode][0] && neg_result == expected[mode][1],
                    "fistp control-word rounding mode %u: %d %d",
                    mode, pos_result, neg_result);
    }

    /* FISTTP ignores the control-word rounding mode and always truncates. */
    cw = (uint16_t)((saved_cw & ~UINT16_C(0x0c00)) | UINT16_C(0x0400));
    negative = -2.9;
    __asm__ volatile("fldcw %0" : : "m"(cw));
    __asm__ volatile("fldl %1\n\tfisttpl %0" : "=m"(neg_result) : "m"(negative));
    TEST_ASSERT(neg_result == -2, "fisttp ignores round-down mode for -2.9");

    int16_t out16;
    int64_t out64;
    uint16_t status;
    double out_of_16 = 32768.0;
    double infinity = INFINITY;
    __asm__ volatile(
        "fnclex\n\tfldl %3\n\tfistps %0\n\t"
        "fldl %4\n\tfistpq %1\n\tfnstsw %2"
        : "=m"(out16), "=m"(out64), "=m"(status)
        : "m"(out_of_16), "m"(infinity)
        : "memory"
    );
    TEST_ASSERT((uint16_t)out16 == UINT16_C(0x8000),
                "fistp 16-bit overflow integer-indefinite");
    TEST_ASSERT((uint64_t)out64 == UINT64_C(0x8000000000000000),
                "fistp 64-bit infinity integer-indefinite");
    TEST_ASSERT(status & 1, "fistp width overflow sets invalid status");

    /* Non-pop FIST leaves the original value on the x87 stack. */
    double retained;
    positive = 3.5;
    cw = (uint16_t)(saved_cw & ~UINT16_C(0x0c00));
    __asm__ volatile("fldcw %0" : : "m"(cw));
    __asm__ volatile(
        "fldl %2\n\tfistl %0\n\tfstpl %1"
        : "=m"(pos_result), "=m"(retained) : "m"(positive)
    );
    TEST_ASSERT(pos_result == 4, "fist non-pop rounded result");
    TEST_ASSERT(retained == 3.5, "fist leaves original ST(0) value on stack");
    __asm__ volatile("fldcw %0" : : "m"(saved_cw));
}

int main(void) {
    TEST_START("FILD/FIST/FISTP/FISTTP instructions");
    test_fild_16();
    test_fild_32();
    test_fild_64();
    test_fist_16();
    test_fistp_32();
    test_fistp_64();
    test_fisttp();
    test_fild_fist_boundaries();
    test_fist_rounding_modes_widths_and_stack();
    TEST_END();
}
