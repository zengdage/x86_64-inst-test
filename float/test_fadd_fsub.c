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
    TEST_ASSERT(IS_QNAN(result), "fadd inf+(-inf) = QNaN");

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
    TEST_ASSERT(IS_QNAN(result), "fsub inf-inf = QNaN");
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

static void test_fadd_fsub_status_precision_and_signed_zero(void) {
    double pos_inf = INFINITY, neg_inf = -INFINITY, result;
    uint16_t status;

    __asm__ volatile (
        "fnclex\n\tfldl %2\n\tfaddl %3\n\tfstpl %0\n\tfnstsw %1"
        : "=m"(result), "=m"(status) : "m"(pos_inf), "m"(neg_inf)
        : "memory"
    );
    TEST_ASSERT(IS_QNAN(result), "fadd +inf + -inf produces QNaN");
    TEST_ASSERT(status & 1, "fadd opposite infinities sets invalid status");

    __asm__ volatile (
        "fnclex\n\tfldl %2\n\tfsubl %3\n\tfstpl %0\n\tfnstsw %1"
        : "=m"(result), "=m"(status) : "m"(pos_inf), "m"(pos_inf)
        : "memory"
    );
    TEST_ASSERT(IS_QNAN(result), "fsub infinity minus itself produces QNaN");
    TEST_ASSERT(status & 1, "fsub equal infinities sets invalid status");

    double one = 1.0, tiny = 0x1p-65;
    __asm__ volatile (
        "fnclex\n\tfldl %2\n\tfaddl %3\n\tfstpl %0\n\tfnstsw %1"
        : "=m"(result), "=m"(status) : "m"(one), "m"(tiny)
        : "memory"
    );
    TEST_ASSERT(result == 1.0, "fadd value below extended-precision half ULP rounds to one");
    TEST_ASSERT(status & (1u << 5), "fadd inexact result sets precision status");

    uint16_t saved_cw, cw;
    double pos_zero = 0.0, neg_zero = -0.0;
    uint64_t result_bits;
    __asm__ volatile("fnstcw %0" : "=m"(saved_cw));
    cw = (uint16_t)((saved_cw & ~UINT16_C(0x0c00)) | UINT16_C(0x0400));
    __asm__ volatile("fldcw %0" : : "m"(cw));
    __asm__ volatile("fldl %1\n\tfaddl %2\n\tfstpl %0"
        : "=m"(result) : "m"(pos_zero), "m"(neg_zero));
    memcpy(&result_bits, &result, sizeof(result_bits));
    TEST_ASSERT(result_bits == UINT64_C(0x8000000000000000),
                "fadd exact opposite zeros rounds to -0 in round-down mode");

    cw = (uint16_t)((saved_cw & ~UINT16_C(0x0c00)) | UINT16_C(0x0800));
    __asm__ volatile("fldcw %0" : : "m"(cw));
    __asm__ volatile("fldl %1\n\tfaddl %2\n\tfstpl %0"
        : "=m"(result) : "m"(pos_zero), "m"(neg_zero));
    memcpy(&result_bits, &result, sizeof(result_bits));
    TEST_ASSERT(result_bits == 0, "fadd exact opposite zeros rounds to +0 in round-up mode");
    __asm__ volatile("fldcw %0\n\tfnclex" : : "m"(saved_cw));
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
    test_fadd_fsub_status_precision_and_signed_zero();
    TEST_END();
}
