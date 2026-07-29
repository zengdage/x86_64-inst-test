/*
 * test_fadd_fsub.c - Test x87 FADD/FADDP/FSUB/FSUBP/FSUBR/FSUBRP instructions
 *
 * FADD: ST(0) = ST(0) + source
 * FADDP: ST(1) = ST(1) + ST(0), then pop
 * FSUB: ST(0) = ST(0) - source
 * FSUBP: ST(1) = ST(1) - ST(0), then pop
 * FSUBR: ST(0) = source - ST(0) (reverse subtract)
 * FSUBRP: ST(1) = ST(0) - ST(1), then pop
 *
 * Compile: gcc -o test_fadd_fsub float/test_fadd_fsub.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_fadd_mem(void) {
    double a, b, result;

    /* Basic addition */
    a = 1.5;
    b = 2.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "faddl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 4.0, "fadd 1.5+2.5: got %f", result);

    /* Add with float (32-bit) memory */
    float fa = 1.0f, fb = 3.0f;
    float fresult;
    __asm__ volatile (
        "flds %1\n\t"
        "fadds %2\n\t"
        "fstps %0"
        : "=m"(fresult)
        : "m"(fa), "m"(fb)
    );
    TEST_ASSERT(fresult == 4.0f, "fadds 1.0+3.0: got %f", fresult);

    /* Add zero */
    a = 42.0;
    b = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "faddl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 42.0, "fadd x+0: got %f", result);

    /* Add infinity */
    a = 1.0;
    b = INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "faddl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isinf(result) && result > 0, "fadd 1+inf = inf");

    /* Inf + (-Inf) = NaN */
    a = INFINITY;
    b = -INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "faddl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isnan(result), "fadd inf+(-inf) = NaN");

    /* Negative numbers */
    a = -10.0;
    b = -20.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "faddl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == -30.0, "fadd -10+(-20): got %f", result);
}

static void test_faddp(void) {
    double a = 100.0, b = 200.0, result;

    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 100.0 */
        "fldl %2\n\t"       /* ST(0) = 200.0, ST(1) = 100.0 */
        "faddp\n\t"         /* ST(0) = 300.0 */
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 300.0, "faddp 100+200: got %f", result);
}

static void test_fsub_mem(void) {
    double a, b, result;

    /* Basic subtraction */
    a = 10.0;
    b = 3.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsubl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 7.0, "fsub 10-3: got %f", result);

    /* Subtract same value (result = 0) */
    a = 42.0;
    b = 42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsubl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 0.0, "fsub x-x = 0: got %f", result);

    /* Subtract producing negative */
    a = 3.0;
    b = 10.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsubl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == -7.0, "fsub 3-10: got %f", result);

    /* Float (32-bit) subtraction */
    float fa = 5.0f, fb = 2.0f, fresult;
    __asm__ volatile (
        "flds %1\n\t"
        "fsubs %2\n\t"
        "fstps %0"
        : "=m"(fresult)
        : "m"(fa), "m"(fb)
    );
    TEST_ASSERT(fresult == 3.0f, "fsubs 5-2: got %f", fresult);

    /* Inf - Inf = NaN */
    a = INFINITY;
    b = INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "fsubl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isnan(result), "fsub inf-inf = NaN");
}

static void test_fsubp(void) {
    double a = 100.0, b = 30.0, result;

    /* FSUBP: in AT&T syntax, fsubp computes ST(0) - ST(1), stores in ST(1), pops */
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 100.0 */
        "fldl %2\n\t"       /* ST(0) = 30.0, ST(1) = 100.0 */
        "fsubp\n\t"         /* ST(0) = 30.0 - 100.0 = -70.0 (AT&T convention) */
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == -70.0, "fsubp (AT&T): got %f", result);
}

static void test_fsubr(void) {
    double a, b, result;

    /* FSUBR: source - ST(0) */
    a = 3.0;
    b = 10.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 3.0 */
        "fsubrl %2\n\t"     /* ST(0) = 10.0 - 3.0 = 7.0 */
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 7.0, "fsubr 10-3: got %f", result);

    /* Float version */
    float fa = 2.0f, fb = 8.0f, fresult;
    __asm__ volatile (
        "flds %1\n\t"
        "fsubrs %2\n\t"
        "fstps %0"
        : "=m"(fresult)
        : "m"(fa), "m"(fb)
    );
    TEST_ASSERT(fresult == 6.0f, "fsubrs 8-2: got %f", fresult);
}

static void test_fsubrp(void) {
    double a = 100.0, b = 30.0, result;

    /* FSUBRP: in AT&T syntax, fsubrp computes ST(1) - ST(0), stores in ST(1), pops */
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 100.0 */
        "fldl %2\n\t"       /* ST(0) = 30.0, ST(1) = 100.0 */
        "fsubrp\n\t"        /* ST(0) = 100.0 - 30.0 = 70.0 (AT&T convention) */
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 70.0, "fsubrp (AT&T): got %f", result);
}

static void test_denormal(void) {
    double a, b, result;

    /* Denormal addition */
    a = DBL_MIN / 2.0;
    b = DBL_MIN / 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "faddl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == DBL_MIN, "fadd denormal+denormal = DBL_MIN");
}

int main(void) {
    TEST_START("FADD/FADDP/FSUB/FSUBP/FSUBR/FSUBRP instructions");
    test_fadd_mem();
    test_faddp();
    test_fsub_mem();
    test_fsubp();
    test_fsubr();
    test_fsubrp();
    test_denormal();
    TEST_END();
}
