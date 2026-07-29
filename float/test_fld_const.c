/*
 * test_fld_const.c - Test x87 constant loading instructions
 *
 * FLD1:   Push +1.0
 * FLDZ:   Push +0.0
 * FLDPI:  Push pi
 * FLDL2E: Push log2(e)
 * FLDL2T: Push log2(10)
 * FLDLG2: Push log10(2)
 * FLDLN2: Push ln(2)
 *
 * Compile: gcc -o test_fld_const float/test_fld_const.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>

int main(void) {
    TEST_START("FLD1/FLDZ/FLDPI/FLDL2E/FLDL2T/FLDLG2/FLDLN2 instructions");

    double result;

    /* FLD1 */
    __asm__ volatile (
        "fld1\n\t"
        "fstpl %0"
        : "=m"(result)
    );
    TEST_ASSERT(result == 1.0, "fld1 = 1.0: got %f", result);

    /* FLDZ */
    __asm__ volatile (
        "fldz\n\t"
        "fstpl %0"
        : "=m"(result)
    );
    TEST_ASSERT(result == 0.0, "fldz = 0.0: got %f", result);

    /* FLDPI */
    __asm__ volatile (
        "fldpi\n\t"
        "fstpl %0"
        : "=m"(result)
    );
    TEST_ASSERT(fabs(result - M_PI) < 1e-15, "fldpi ~ pi: got %.18f", result);

    /* FLDL2E: log2(e) */
    __asm__ volatile (
        "fldl2e\n\t"
        "fstpl %0"
        : "=m"(result)
    );
    TEST_ASSERT(fabs(result - M_LOG2E) < 1e-15, "fldl2e ~ log2(e): got %.18f", result);

    /* FLDL2T: log2(10) */
    __asm__ volatile (
        "fldl2t\n\t"
        "fstpl %0"
        : "=m"(result)
    );
    double log2_10 = log2(10.0);
    TEST_ASSERT(fabs(result - log2_10) < 1e-15, "fldl2t ~ log2(10): got %.18f", result);

    /* FLDLG2: log10(2) */
    __asm__ volatile (
        "fldlg2\n\t"
        "fstpl %0"
        : "=m"(result)
    );
    double log10_2 = log10(2.0);
    TEST_ASSERT(fabs(result - log10_2) < 1e-15,
                "fldlg2 ~ log10(2): got %.18f, expected %.18f", result, log10_2);

    /* FLDLN2: ln(2) */
    __asm__ volatile (
        "fldln2\n\t"
        "fstpl %0"
        : "=m"(result)
    );
    TEST_ASSERT(fabs(result - M_LN2) < 1e-15, "fldln2 ~ ln(2): got %.18f", result);

    /* Verify relative relationships */
    double fpu_pi, fpu_l2e, fpu_l2t, fpu_lg2, fpu_ln2;
    __asm__ volatile (
        "fldpi\n\t"
        "fstpl %0\n\t"
        "fldl2e\n\t"
        "fstpl %1\n\t"
        "fldl2t\n\t"
        "fstpl %2\n\t"
        "fldlg2\n\t"
        "fstpl %3\n\t"
        "fldln2\n\t"
        "fstpl %4"
        : "=m"(fpu_pi), "=m"(fpu_l2e), "=m"(fpu_l2t), "=m"(fpu_lg2), "=m"(fpu_ln2)
    );
    /* log2(e) * ln(2) = 1 */
    TEST_ASSERT(fabs(fpu_l2e * fpu_ln2 - 1.0) < 1e-14,
                "log2(e) * ln(2) ~ 1: got %.15f", fpu_l2e * fpu_ln2);
    /* log2(10) * log10(2) = 1 */
    TEST_ASSERT(fabs(fpu_l2t * fpu_lg2 - 1.0) < 1e-14,
                "log2(10) * log10(2) ~ 1: got %.15f", fpu_l2t * fpu_lg2);
    /* pi > 3.14 and pi < 3.15 */
    TEST_ASSERT(fpu_pi > 3.14 && fpu_pi < 3.15, "pi in range [3.14, 3.15]");

    TEST_END();
}
