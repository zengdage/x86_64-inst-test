/*
 * test_fmul_fdiv.c - Test x87 FMUL/FMULP/FDIV/FDIVP/FDIVR/FDIVRP instructions
 *
 * FMUL: ST(0) = ST(0) * source
 * FMULP: ST(1) = ST(1) * ST(0), then pop
 * FDIV: ST(0) = ST(0) / source
 * FDIVP: ST(1) = ST(1) / ST(0), then pop
 * FDIVR: ST(0) = source / ST(0) (reverse divide)
 * FDIVRP: ST(1) = ST(0) / ST(1), then pop
 *
 * Compile: gcc -o test_fmul_fdiv float/test_fmul_fdiv.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_fmul_mem(void) {
    double a, b, result;

    /* Basic multiply */
    a = 3.0;
    b = 4.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 12.0, "fmul 3*4: got %f", result);

    /* Multiply by zero */
    a = 42.0;
    b = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 0.0, "fmul x*0: got %f", result);

    /* Multiply by one */
    a = 123.456;
    b = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 123.456, "fmul x*1: got %f", result);

    /* Multiply negatives */
    a = -3.0;
    b = -5.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 15.0, "fmul (-3)*(-5): got %f", result);

    /* Multiply negative and positive */
    a = -3.0;
    b = 5.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == -15.0, "fmul (-3)*5: got %f", result);

    /* Inf * 0 = NaN */
    a = INFINITY;
    b = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isnan(result), "fmul inf*0 = NaN");

    /* Inf * finite = Inf */
    a = INFINITY;
    b = 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isinf(result) && result > 0, "fmul inf*2 = inf");

    /* Float 32-bit */
    float fa = 2.5f, fb = 4.0f, fresult;
    __asm__ volatile (
        "flds %1\n\t"
        "fmuls %2\n\t"
        "fstps %0"
        : "=m"(fresult)
        : "m"(fa), "m"(fb)
    );
    TEST_ASSERT(fresult == 10.0f, "fmuls 2.5*4: got %f", fresult);
}

static void test_fmulp(void) {
    double a = 7.0, b = 8.0, result;

    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fmulp\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 56.0, "fmulp 7*8: got %f", result);
}

static void test_fdiv_mem(void) {
    double a, b, result;

    /* Basic division */
    a = 10.0;
    b = 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 5.0, "fdiv 10/2: got %f", result);

    /* Division by one */
    a = 42.0;
    b = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 42.0, "fdiv x/1: got %f", result);

    /* 0 / x = 0 */
    a = 0.0;
    b = 5.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 0.0, "fdiv 0/5: got %f", result);

    /* x / 0 = Inf */
    a = 1.0;
    b = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isinf(result) && result > 0, "fdiv 1/0 = +inf");

    /* -x / 0 = -Inf */
    a = -1.0;
    b = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isinf(result) && result < 0, "fdiv -1/0 = -inf");

    /* 0 / 0 = NaN */
    a = 0.0;
    b = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isnan(result), "fdiv 0/0 = NaN");

    /* Inf / Inf = NaN */
    a = INFINITY;
    b = INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(isnan(result), "fdiv inf/inf = NaN");

    /* Float 32-bit */
    float fa = 9.0f, fb = 3.0f, fresult;
    __asm__ volatile (
        "flds %1\n\t"
        "fdivs %2\n\t"
        "fstps %0"
        : "=m"(fresult)
        : "m"(fa), "m"(fb)
    );
    TEST_ASSERT(fresult == 3.0f, "fdivs 9/3: got %f", fresult);
}

static void test_fdivp(void) {
    double a = 100.0, b = 25.0, result;

    /* FDIVP: in AT&T syntax, fdivp computes ST(0) / ST(1), stores in ST(1), pops */
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 100.0 */
        "fldl %2\n\t"       /* ST(0) = 25.0, ST(1) = 100.0 */
        "fdivp\n\t"         /* ST(0) = 25.0 / 100.0 = 0.25 (AT&T convention) */
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 0.25, "fdivp (AT&T): got %f", result);
}

static void test_fdivr(void) {
    double a, b, result;

    /* FDIVR: source / ST(0) */
    a = 2.0;
    b = 10.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 2.0 */
        "fdivrl %2\n\t"     /* ST(0) = 10.0 / 2.0 = 5.0 */
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 5.0, "fdivr 10/2: got %f", result);

    /* Float */
    float fa = 4.0f, fb = 20.0f, fresult;
    __asm__ volatile (
        "flds %1\n\t"
        "fdivrs %2\n\t"
        "fstps %0"
        : "=m"(fresult)
        : "m"(fa), "m"(fb)
    );
    TEST_ASSERT(fresult == 5.0f, "fdivrs 20/4: got %f", fresult);
}

static void test_fdivrp(void) {
    double a = 100.0, b = 25.0, result;

    /* FDIVRP: in AT&T syntax, fdivrp computes ST(1) / ST(0), stores in ST(1), pops */
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 100.0 */
        "fldl %2\n\t"       /* ST(0) = 25.0, ST(1) = 100.0 */
        "fdivrp\n\t"        /* ST(0) = 100.0 / 25.0 = 4.0 (AT&T convention) */
        "fstpl %0"
        : "=m"(result)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(result == 4.0, "fdivrp (AT&T): got %f", result);
}

static void test_denormal(void) {
    double result;
    double denorm = DBL_MIN / 2.0;
    double two = 2.0;

    __asm__ volatile (
        "fldl %1\n\t"
        "fmull %2\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(denorm), "m"(two)
    );
    TEST_ASSERT(result == DBL_MIN, "fmul denormal*2 = DBL_MIN");
}

int main(void) {
    TEST_START("FMUL/FMULP/FDIV/FDIVP/FDIVR/FDIVRP instructions");
    test_fmul_mem();
    test_fmulp();
    test_fdiv_mem();
    test_fdivp();
    test_fdivr();
    test_fdivrp();
    test_denormal();
    TEST_END();
}
