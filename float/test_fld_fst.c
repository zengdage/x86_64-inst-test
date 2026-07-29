/*
 * test_fld_fst.c - Test x87 FLD/FST/FSTP instructions
 *
 * FLD pushes a floating-point value onto the x87 FPU stack.
 * FST stores ST(0) to memory without popping.
 * FSTP stores ST(0) to memory and pops the stack.
 * Supports float (32-bit), double (64-bit), and long double (80-bit) operands.
 *
 * Compile: gcc -o test_fld_fst float/test_fld_fst.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_fld_fst_float(void) {
    float src, dst;

    /* Basic positive value */
    src = 3.14f;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 3.14f, "fld/fstp float 3.14: got %f", dst);

    /* Zero */
    src = 0.0f;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 0.0f, "fld/fstp float 0.0: got %f", dst);

    /* Negative zero */
    src = -0.0f;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 0.0f && signbit(dst), "fld/fstp float -0.0: got %f", dst);

    /* Positive infinity */
    src = INFINITY;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(isinf(dst) && dst > 0, "fld/fstp float +inf");

    /* Negative infinity */
    src = -INFINITY;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(isinf(dst) && dst < 0, "fld/fstp float -inf");

    /* NaN */
    src = NAN;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(isnan(dst), "fld/fstp float NaN");

    /* Denormal */
    src = FLT_MIN / 2.0f;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == FLT_MIN / 2.0f, "fld/fstp float denormal");

    /* Max float */
    src = FLT_MAX;
    __asm__ volatile (
        "flds %1\n\t"
        "fstps %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == FLT_MAX, "fld/fstp float FLT_MAX");

    /* FST (non-popping) */
    src = 42.0f;
    float dst2;
    __asm__ volatile (
        "flds %2\n\t"
        "fsts %0\n\t"
        "fstps %1"
        : "=m"(dst), "=m"(dst2)
        : "m"(src)
    );
    TEST_ASSERT(dst == 42.0f, "fst float: got %f", dst);
    TEST_ASSERT(dst2 == 42.0f, "fstp after fst: got %f", dst2);
}

static void test_fld_fst_double(void) {
    double src, dst;

    /* Basic positive */
    src = 2.718281828459045;
    __asm__ volatile (
        "fldl %1\n\t"
        "fstpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 2.718281828459045, "fld/fstp double e: got %.15f", dst);

    /* Zero */
    src = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fstpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 0.0, "fld/fstp double 0.0");

    /* Negative */
    src = -1.0e308;
    __asm__ volatile (
        "fldl %1\n\t"
        "fstpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == -1.0e308, "fld/fstp double -1e308");

    /* Infinity */
    src = INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "fstpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(isinf(dst), "fld/fstp double +inf");

    /* NaN */
    src = NAN;
    __asm__ volatile (
        "fldl %1\n\t"
        "fstpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(isnan(dst), "fld/fstp double NaN");

    /* Denormal */
    src = DBL_MIN / 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fstpl %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == DBL_MIN / 2.0, "fld/fstp double denormal");

    /* FST double (non-popping) */
    src = 99.99;
    double dst2;
    __asm__ volatile (
        "fldl %2\n\t"
        "fstl %0\n\t"
        "fstpl %1"
        : "=m"(dst), "=m"(dst2)
        : "m"(src)
    );
    TEST_ASSERT(dst == 99.99, "fst double: got %f", dst);
    TEST_ASSERT(dst2 == 99.99, "fstp after fst double: got %f", dst2);
}

static void test_fld_fst_long_double(void) {
    long double src, dst;

    /* Basic positive */
    src = 1.23456789012345678L;
    __asm__ volatile (
        "fldt %1\n\t"
        "fstpt %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 1.23456789012345678L, "fld/fstp long double");

    /* Zero */
    src = 0.0L;
    __asm__ volatile (
        "fldt %1\n\t"
        "fstpt %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(dst == 0.0L, "fld/fstp long double 0.0");

    /* Negative infinity */
    long double neg_inf = -1.0L / 0.0L;
    src = neg_inf;
    __asm__ volatile (
        "fldt %1\n\t"
        "fstpt %0"
        : "=m"(dst)
        : "m"(src)
    );
    TEST_ASSERT(isinf(dst) && dst < 0, "fld/fstp long double -inf");
}

static void test_fld_st(void) {
    /* FLD ST(i) - push a copy of ST(i) */
    double a = 10.0, b = 20.0;
    double out1, out2;
    __asm__ volatile (
        "fldl %2\n\t"       /* ST(0) = 10.0 */
        "fldl %3\n\t"       /* ST(0) = 20.0, ST(1) = 10.0 */
        "fld %%st(1)\n\t"   /* ST(0) = 10.0, ST(1) = 20.0, ST(2) = 10.0 */
        "fstpl %0\n\t"      /* out1 = 10.0, pop */
        "fstpl %1\n\t"      /* out2 = 20.0, pop */
        "fstp %%st(0)\n\t"  /* clean up */
        : "=m"(out1), "=m"(out2)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(out1 == 10.0, "fld st(1) copy: got %f, expected 10.0", out1);
    TEST_ASSERT(out2 == 20.0, "remaining st after fld st(1): got %f, expected 20.0", out2);
}

int main(void) {
    TEST_START("FLD/FST/FSTP instructions");
    test_fld_fst_float();
    test_fld_fst_double();
    test_fld_fst_long_double();
    test_fld_st();
    TEST_END();
}
