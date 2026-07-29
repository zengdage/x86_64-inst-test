/*
 * test_fmisc.c - Test x87 miscellaneous instructions
 *
 * FXCH: Exchange ST(0) and ST(i)
 * FFREE: Mark ST(i) as empty
 * FINCSTP: Increment FPU stack pointer
 * FDECSTP: Decrement FPU stack pointer
 * FNCLEX: Clear FPU exception flags
 * FNINIT: Initialize FPU
 * FNSTCW: Store FPU control word
 * FLDCW: Load FPU control word
 * FNSTSW: Store FPU status word
 * FXAM: Examine ST(0) class
 * FPREM: Partial remainder (IEEE-incompatible)
 * FPREM1: IEEE partial remainder
 * FXTRACT: Extract exponent and significand
 *
 * Compile: gcc -o test_fmisc float/test_fmisc.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_fxch(void) {
    double a = 10.0, b = 20.0;
    double out1, out2;

    __asm__ volatile (
        "fldl %2\n\t"       /* ST(0) = 10.0 */
        "fldl %3\n\t"       /* ST(0) = 20.0, ST(1) = 10.0 */
        "fxch %%st(1)\n\t"  /* ST(0) = 10.0, ST(1) = 20.0 */
        "fstpl %0\n\t"
        "fstpl %1"
        : "=m"(out1), "=m"(out2)
        : "m"(a), "m"(b)
    );
    TEST_ASSERT(out1 == 10.0, "fxch: ST(0) after swap = 10: got %f", out1);
    TEST_ASSERT(out2 == 20.0, "fxch: ST(1) after swap = 20: got %f", out2);
}

static void test_fincstp_fdecstp(void) {
    /* FINCSTP/FDECSTP rotate the stack pointer without changing values */
    double val = 42.0, result;

    __asm__ volatile (
        "fldl %1\n\t"
        "fincstp\n\t"       /* increment TOP, now our value is in ST(7) */
        "fdecstp\n\t"       /* decrement TOP, value back in ST(0) */
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 42.0, "fincstp/fdecstp roundtrip: got %f", result);
}

static void test_fnclex(void) {
    uint16_t sw_before, sw_after;

    /* Generate an exception (0/0), then clear it */
    double zero = 0.0, result;
    __asm__ volatile (
        "fldl %2\n\t"
        "fdivl %2\n\t"        /* 0/0 => NaN, sets exception */
        "fstpl %3\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fnclex\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %1"
        : "=m"(sw_before), "=m"(sw_after)
        : "m"(zero), "m"(result)
        : "ax"
    );
    /* After FNCLEX, exception flags should be cleared */
    TEST_ASSERT((sw_after & 0x3F) == 0, "fnclex clears exception flags: sw=0x%04x", sw_after);
}

static void test_fninit(void) {
    uint16_t cw, sw;

    __asm__ volatile (
        "fninit\n\t"
        "fnstcw %0\n\t"
        "fnstsw %1"
        : "=m"(cw), "=m"(sw)
    );
    /* After FNINIT: CW=0x037F, SW=0x0000 */
    TEST_ASSERT(cw == 0x037F, "fninit: CW=0x037F, got 0x%04x", cw);
    TEST_ASSERT(sw == 0x0000, "fninit: SW=0x0000, got 0x%04x", sw);
}

static void test_fnstcw_fldcw(void) {
    uint16_t cw_orig, cw_mod, cw_restored;

    __asm__ volatile (
        "fninit\n\t"
        "fnstcw %0"
        : "=m"(cw_orig)
    );
    TEST_ASSERT(cw_orig == 0x037F, "fnstcw default: 0x%04x", cw_orig);

    /* Change rounding mode to truncate (bits 10-11 = 11b) */
    cw_mod = (cw_orig & ~0x0C00) | 0x0C00;
    __asm__ volatile (
        "fldcw %0"
        :
        : "m"(cw_mod)
    );

    uint16_t cw_check;
    __asm__ volatile (
        "fnstcw %0"
        : "=m"(cw_check)
    );
    TEST_ASSERT(cw_check == cw_mod, "fldcw sets truncation mode: 0x%04x", cw_check);

    /* Test that truncation rounding actually works */
    double val = 2.9, result;
    __asm__ volatile (
        "fldl %1\n\t"
        "frndint\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == 2.0, "truncation rounding 2.9 => 2: got %f", result);

    /* Restore default */
    __asm__ volatile (
        "fldcw %0"
        :
        : "m"(cw_orig)
    );
    __asm__ volatile (
        "fnstcw %0"
        : "=m"(cw_restored)
    );
    TEST_ASSERT(cw_restored == cw_orig, "fldcw restore: 0x%04x", cw_restored);
}

static void test_fnstsw(void) {
    uint16_t sw;

    __asm__ volatile (
        "fninit\n\t"
        "fnstsw %0"
        : "=m"(sw)
    );
    TEST_ASSERT(sw == 0, "fnstsw after fninit: 0x%04x", sw);

    /* FNSTSW AX form */
    uint16_t sw_ax;
    __asm__ volatile (
        "fninit\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0"
        : "=m"(sw_ax)
        :
        : "ax"
    );
    TEST_ASSERT(sw_ax == 0, "fnstsw %%ax after fninit: 0x%04x", sw_ax);
}

static void test_fxam(void) {
    double val;
    uint16_t sw;

    /* Positive normal */
    val = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fxam\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    /* C3=0, C2=1, C0=0, C1=0 (positive normal) => C2 bit at position 10 */
    TEST_ASSERT((sw & 0x4700) == 0x0400, "fxam +normal: C3C2C0=010, C1=0, sw=0x%04x", sw);

    /* Zero */
    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fxam\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    /* C3=1, C2=0, C0=0 (zero) */
    TEST_ASSERT((sw & 0x4700) == 0x4000, "fxam +0: C3C2C0=100, sw=0x%04x", sw);

    /* NaN */
    val = NAN;
    __asm__ volatile (
        "fldl %1\n\t"
        "fxam\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    /* C3=0, C2=0, C0=1 (NaN) */
    TEST_ASSERT((sw & 0x4500) == 0x0100, "fxam NaN: C3C2C0=001, sw=0x%04x", sw);

    /* Infinity */
    val = INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "fxam\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    /* C3=0, C2=1, C0=1 (infinity) */
    TEST_ASSERT((sw & 0x4500) == 0x0500, "fxam +inf: C3C2C0=011, sw=0x%04x", sw);

    /* Negative normal: C1=1 */
    val = -1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fxam\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    /* C3=0, C2=1, C0=0, C1=1 (negative normal) */
    TEST_ASSERT((sw & 0x4700) == 0x0600, "fxam -normal: C1=1, sw=0x%04x", sw);

    /* Denormal: use a true 80-bit extended precision denormal.
     * A double denormal becomes normal when loaded into the x87 80-bit register.
     * We construct a tiny extended-precision denormal via FXTRACT trick. */
    /* Use the smallest positive long double denormal */
    long double ld_denorm;
    /* Build an 80-bit denormal: exponent=0, integer bit=0, significand!=0 */
    uint8_t *p = (uint8_t *)&ld_denorm;
    memset(p, 0, sizeof(long double));
    p[0] = 1;  /* Smallest denormal: exponent=0, significand=1 */
    __asm__ volatile (
        "fldt %1\n\t"
        "fxam\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(ld_denorm)
        : "ax"
    );
    /* C3=1, C2=1, C0=0 (denormal) */
    TEST_ASSERT((sw & 0x4500) == 0x4400, "fxam denormal: C3C2C0=110, sw=0x%04x", sw);
}

static void test_fprem(void) {
    double dividend, divisor, result;

    /* 10.0 mod 3.0 = 1.0 */
    dividend = 10.0;
    divisor = 3.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = divisor */
        "fldl %2\n\t"       /* ST(0) = dividend, ST(1) = divisor */
        "fprem\n\t"
        "fstpl %0\n\t"
        "fstp %%st(0)"
        : "=m"(result)
        : "m"(divisor), "m"(dividend)
    );
    TEST_ASSERT(result == 1.0, "fprem 10 mod 3 = 1: got %f", result);

    /* 7.5 mod 2.0 = 1.5 */
    dividend = 7.5;
    divisor = 2.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fprem\n\t"
        "fstpl %0\n\t"
        "fstp %%st(0)"
        : "=m"(result)
        : "m"(divisor), "m"(dividend)
    );
    TEST_ASSERT(result == 1.5, "fprem 7.5 mod 2 = 1.5: got %f", result);
}

static void test_fprem1(void) {
    double dividend, divisor, result;

    /* IEEE remainder: 10.0 rem 3.0 = 1.0 */
    dividend = 10.0;
    divisor = 3.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fprem1\n\t"
        "fstpl %0\n\t"
        "fstp %%st(0)"
        : "=m"(result)
        : "m"(divisor), "m"(dividend)
    );
    TEST_ASSERT(result == 1.0, "fprem1 10 rem 3 = 1: got %f", result);

    /* IEEE remainder: 7.0 rem 4.0 = -1.0 (rounds quotient to nearest) */
    dividend = 7.0;
    divisor = 4.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fprem1\n\t"
        "fstpl %0\n\t"
        "fstp %%st(0)"
        : "=m"(result)
        : "m"(divisor), "m"(dividend)
    );
    TEST_ASSERT(result == -1.0, "fprem1 7 rem 4 = -1 (IEEE): got %f", result);
}

static void test_fxtract(void) {
    double val;
    double sig, exp;

    /* Extract 8.0 = 1.0 * 2^3 */
    val = 8.0;
    __asm__ volatile (
        "fldl %2\n\t"
        "fxtract\n\t"       /* ST(0) = significand, ST(1) = exponent */
        "fstpl %0\n\t"      /* significand */
        "fstpl %1"           /* exponent */
        : "=m"(sig), "=m"(exp)
        : "m"(val)
    );
    TEST_ASSERT(sig == 1.0, "fxtract(8) significand = 1.0: got %f", sig);
    TEST_ASSERT(exp == 3.0, "fxtract(8) exponent = 3: got %f", exp);

    /* Extract 1.0 = 1.0 * 2^0 */
    val = 1.0;
    __asm__ volatile (
        "fldl %2\n\t"
        "fxtract\n\t"
        "fstpl %0\n\t"
        "fstpl %1"
        : "=m"(sig), "=m"(exp)
        : "m"(val)
    );
    TEST_ASSERT(sig == 1.0, "fxtract(1) significand = 1.0: got %f", sig);
    TEST_ASSERT(exp == 0.0, "fxtract(1) exponent = 0: got %f", exp);

    /* Extract 0.5 = 1.0 * 2^(-1) */
    val = 0.5;
    __asm__ volatile (
        "fldl %2\n\t"
        "fxtract\n\t"
        "fstpl %0\n\t"
        "fstpl %1"
        : "=m"(sig), "=m"(exp)
        : "m"(val)
    );
    TEST_ASSERT(sig == 1.0, "fxtract(0.5) significand = 1.0: got %f", sig);
    TEST_ASSERT(exp == -1.0, "fxtract(0.5) exponent = -1: got %f", exp);

    /* Extract 12.0 = 1.5 * 2^3 */
    val = 12.0;
    __asm__ volatile (
        "fldl %2\n\t"
        "fxtract\n\t"
        "fstpl %0\n\t"
        "fstpl %1"
        : "=m"(sig), "=m"(exp)
        : "m"(val)
    );
    TEST_ASSERT(sig == 1.5, "fxtract(12) significand = 1.5: got %f", sig);
    TEST_ASSERT(exp == 3.0, "fxtract(12) exponent = 3: got %f", exp);
}

static void test_ffree(void) {
    uint16_t sw;
    /* FFREE marks a register as empty; we verify via FXAM on an empty slot */
    double val = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "ffree %%st(0)\n\t"
        "fxam\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fincstp"            /* move past the freed slot */
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    /* Empty: C3=1, C2=0, C0=1 */
    TEST_ASSERT((sw & 0x4500) == 0x4100, "ffree then fxam: empty (C3C2C0=101), sw=0x%04x", sw);
}

int main(void) {
    TEST_START("FXCH/FFREE/FINCSTP/FDECSTP/FNCLEX/FNINIT/FNSTCW/FLDCW/FNSTSW/FXAM/FPREM/FPREM1/FXTRACT instructions");
    test_fxch();
    test_fincstp_fdecstp();
    test_fnclex();
    test_fninit();
    test_fnstcw_fldcw();
    test_fnstsw();
    test_fxam();
    test_fprem();
    test_fprem1();
    test_fxtract();
    test_ffree();
    TEST_END();
}
