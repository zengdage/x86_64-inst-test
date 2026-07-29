/*
 * test_fmath.c - Test x87 math instructions
 *
 * FSQRT: ST(0) = sqrt(ST(0))
 * FABS: ST(0) = |ST(0)|
 * FCHS: ST(0) = -ST(0)
 * FRNDINT: ST(0) = round to integer (using current rounding mode)
 * FSIN: ST(0) = sin(ST(0))
 * FCOS: ST(0) = cos(ST(0))
 * FPTAN: ST(0) = tan(ST(0)), pushes 1.0
 * FPATAN: ST(0) = atan2(ST(1), ST(0)), pops
 * F2XM1: ST(0) = 2^ST(0) - 1, for -1 <= ST(0) <= 1
 * FSCALE: ST(0) = ST(0) * 2^trunc(ST(1))
 * FYL2X: ST(0) = ST(1) * log2(ST(0)), pops
 *
 * Compile: gcc -o test_fmath float/test_fmath.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_fsqrt(void) {
    double val, result;

    val = 4.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsqrt\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 2.0, "fsqrt(4) = 2: got %f", result);

    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsqrt\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 0.0, "fsqrt(0) = 0");

    val = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsqrt\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 1.0, "fsqrt(1) = 1");

    val = 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsqrt\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result - sqrt(2.0)) < 1e-15, "fsqrt(2): got %.15f", result);

    /* sqrt(inf) = inf */
    val = INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsqrt\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(isinf(result) && result > 0, "fsqrt(inf) = inf");

    /* sqrt(-1) = NaN */
    val = -1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsqrt\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(isnan(result), "fsqrt(-1) = NaN");
}

static void test_fabs(void) {
    double val, result;

    val = -42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fabs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 42.0, "fabs(-42) = 42: got %f", result);

    val = 42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fabs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 42.0, "fabs(42) = 42");

    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fabs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 0.0, "fabs(0) = 0");

    val = -0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fabs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 0.0 && !signbit(result), "fabs(-0) = +0");

    val = -INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "fabs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(isinf(result) && result > 0, "fabs(-inf) = +inf");
}

static void test_fchs(void) {
    double val, result;

    val = 42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fchs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == -42.0, "fchs(42) = -42");

    val = -42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fchs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 42.0, "fchs(-42) = 42");

    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fchs\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 0.0 && signbit(result), "fchs(+0) = -0");
}

static void test_frndint(void) {
    double val, result;

    /* Default rounding mode is round-to-nearest-even */
    val = 2.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "frndint\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 2.0, "frndint(2.5) = 2 (banker's): got %f", result);

    val = 3.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "frndint\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 4.0, "frndint(3.5) = 4 (banker's): got %f", result);

    val = -2.7;
    __asm__ volatile (
        "fldl %1\n\t"
        "frndint\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == -3.0, "frndint(-2.7) = -3: got %f", result);

    val = 7.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "frndint\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 7.0, "frndint(7.0) = 7");
}

static void test_fsin(void) {
    double val, result;

    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsin\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 0.0, "fsin(0) = 0");

    /* sin(pi/2) ~ 1.0 */
    val = M_PI / 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsin\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result - 1.0) < 1e-14, "fsin(pi/2) ~ 1.0: got %.15f", result);

    /* sin(pi) ~ 0 */
    val = M_PI;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsin\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result) < 1e-14, "fsin(pi) ~ 0: got %.15e", result);
}

static void test_fcos(void) {
    double val, result;

    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fcos\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 1.0, "fcos(0) = 1");

    /* cos(pi) = -1 */
    val = M_PI;
    __asm__ volatile (
        "fldl %1\n\t"
        "fcos\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result - (-1.0)) < 1e-14, "fcos(pi) ~ -1: got %.15f", result);

    /* cos(pi/2) ~ 0 */
    val = M_PI / 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fcos\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result) < 1e-14, "fcos(pi/2) ~ 0: got %.15e", result);
}

static void test_fptan(void) {
    double val, tan_result, one_result;

    /* FPTAN: replaces ST(0) with tan(ST(0)) and pushes 1.0 */
    val = M_PI / 4.0;
    __asm__ volatile (
        "fldl %2\n\t"
        "fptan\n\t"
        "fstpl %1\n\t"     /* pop 1.0 */
        "fstpl %0"          /* pop tan result */
        : "=m"(tan_result), "=m"(one_result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(tan_result - 1.0) < 1e-14, "fptan(pi/4) ~ 1.0: got %.15f", tan_result);
    TEST_ASSERT(one_result == 1.0, "fptan pushes 1.0: got %f", one_result);

    /* tan(0) = 0 */
    val = 0.0;
    __asm__ volatile (
        "fldl %2\n\t"
        "fptan\n\t"
        "fstpl %1\n\t"
        "fstpl %0"
        : "=m"(tan_result), "=m"(one_result)
        : "m"(val)
    );
    TEST_ASSERT(tan_result == 0.0, "fptan(0) = 0");
}

static void test_fpatan(void) {
    double y, x, result;

    /* atan2(1, 1) = pi/4 */
    y = 1.0;
    x = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = y */
        "fldl %2\n\t"       /* ST(0) = x, ST(1) = y */
        "fpatan\n\t"         /* ST(0) = atan2(y, x) */
        "fstpl %0"
        : "=m"(result)
        : "m"(y), "m"(x)
    );
    TEST_ASSERT(fabs(result - M_PI / 4.0) < 1e-14, "fpatan(1,1) ~ pi/4: got %.15f", result);

    /* atan2(0, 1) = 0 */
    y = 0.0;
    x = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fpatan\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(y), "m"(x)
    );
    TEST_ASSERT(result == 0.0, "fpatan(0,1) = 0");
}

static void test_f2xm1(void) {
    double val, result;

    /* f2xm1(0) = 2^0 - 1 = 0 */
    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "f2xm1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 0.0, "f2xm1(0) = 0");

    /* f2xm1(1) = 2^1 - 1 = 1 */
    val = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "f2xm1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result - 1.0) < 1e-14, "f2xm1(1) = 1: got %.15f", result);

    /* f2xm1(-1) = 2^(-1) - 1 = -0.5 */
    val = -1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "f2xm1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result - (-0.5)) < 1e-14, "f2xm1(-1) = -0.5: got %.15f", result);

    /* f2xm1(0.5) = 2^0.5 - 1 ~ 0.4142... */
    val = 0.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "f2xm1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(fabs(result - (sqrt(2.0) - 1.0)) < 1e-14, "f2xm1(0.5) ~ sqrt(2)-1: got %.15f", result);
}

static void test_fscale(void) {
    double val, scale, result;

    /* 1.0 * 2^3 = 8.0 */
    val = 1.0;
    scale = 3.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = scale = 3.0 */
        "fldl %2\n\t"       /* ST(0) = val = 1.0, ST(1) = 3.0 */
        "fscale\n\t"         /* ST(0) = 1.0 * 2^3 = 8.0 */
        "fstpl %0\n\t"
        "fstp %%st(0)"       /* clean ST(1) */
        : "=m"(result)
        : "m"(scale), "m"(val)
    );
    TEST_ASSERT(result == 8.0, "fscale 1*2^3 = 8: got %f", result);

    /* 5.0 * 2^(-1) = 2.5 */
    val = 5.0;
    scale = -1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fscale\n\t"
        "fstpl %0\n\t"
        "fstp %%st(0)"
        : "=m"(result)
        : "m"(scale), "m"(val)
    );
    TEST_ASSERT(result == 2.5, "fscale 5*2^(-1) = 2.5: got %f", result);
}

static void test_fyl2x(void) {
    double y, x, result;

    /* 1 * log2(8) = 3 */
    y = 1.0;
    x = 8.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = y */
        "fldl %2\n\t"       /* ST(0) = x, ST(1) = y */
        "fyl2x\n\t"         /* ST(0) = y * log2(x) */
        "fstpl %0"
        : "=m"(result)
        : "m"(y), "m"(x)
    );
    TEST_ASSERT(fabs(result - 3.0) < 1e-14, "fyl2x 1*log2(8) = 3: got %.15f", result);

    /* 2 * log2(4) = 4 */
    y = 2.0;
    x = 4.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fyl2x\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(y), "m"(x)
    );
    TEST_ASSERT(fabs(result - 4.0) < 1e-14, "fyl2x 2*log2(4) = 4: got %.15f", result);

    /* log2(1) = 0 */
    y = 1.0;
    x = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fyl2x\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(y), "m"(x)
    );
    TEST_ASSERT(result == 0.0, "fyl2x 1*log2(1) = 0");
}

int main(void) {
    TEST_START("FSQRT/FABS/FCHS/FRNDINT/FSIN/FCOS/FPTAN/FPATAN/F2XM1/FSCALE/FYL2X instructions");
    test_fsqrt();
    test_fabs();
    test_fchs();
    test_frndint();
    test_fsin();
    test_fcos();
    test_fptan();
    test_fpatan();
    test_f2xm1();
    test_fscale();
    test_fyl2x();
    TEST_END();
}
