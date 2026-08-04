/*
 * test_fcmp.c - Test x87 comparison instructions
 *
 * FCOM/FCOMP/FCOMPP: Compare ST(0) with operand, set C0/C2/C3 in FPU status word.
 * FCOMI/FCOMIP: Compare ST(0) with ST(i), set EFLAGS (CF, ZF, PF) directly.
 * FUCOMI/FUCOMIP: Unordered compare (like FCOMI but does not raise #IA for QNaN).
 * FTST: Compare ST(0) with 0.0.
 *
 * Compile: gcc -o test_fcmp float/test_fcmp.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_fcom(void) {
    double a, b;
    uint16_t sw;

    /* ST(0) > mem: C3=0, C2=0, C0=0 */
    a = 10.0;
    b = 5.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fcoml %2\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(a), "m"(b)
        : "ax"
    );
    TEST_ASSERT(!(sw & 0x4500), "fcom 10>5: C3=C2=C0=0, sw=0x%04x", sw);

    /* ST(0) < mem: C0=1 */
    a = 3.0;
    b = 7.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fcoml %2\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(a), "m"(b)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x0100, "fcom 3<7: C0=1, sw=0x%04x", sw);

    /* ST(0) == mem: C3=1 */
    a = 5.0;
    b = 5.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fcoml %2\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(a), "m"(b)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x4000, "fcom 5==5: C3=1, sw=0x%04x", sw);

    /* Unordered (NaN): C3=1, C2=1, C0=1 */
    a = NAN;
    b = 1.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fcoml %2\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(a), "m"(b)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x4500, "fcom NaN: unordered, sw=0x%04x", sw);
}

static void test_fcomp(void) {
    double a = 10.0, b = 5.0;
    uint16_t sw;

    /* FCOMP pops ST(0) after compare */
    __asm__ volatile (
        "fldl %1\n\t"
        "fcompl %2\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0"
        : "=m"(sw)
        : "m"(a), "m"(b)
        : "ax"
    );
    TEST_ASSERT(!(sw & 0x4500), "fcomp 10>5: C3=C2=C0=0");
}

static void test_fcompp(void) {
    double a = 3.0, b = 3.0;
    uint16_t sw;

    /* FCOMPP pops both ST(0) and ST(1) */
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 3.0 */
        "fldl %2\n\t"       /* ST(0) = 3.0, ST(1) = 3.0 */
        "fcompp\n\t"        /* compare and pop both */
        "fnstsw %%ax\n\t"
        "movw %%ax, %0"
        : "=m"(sw)
        : "m"(a), "m"(b)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x4000, "fcompp 3==3: C3=1");
}

static void test_fcomi(void) {
    double a, b;
    uint64_t flags;

    /* FCOMI sets EFLAGS directly: CF, ZF, PF */
    /* ST(0) > ST(1): CF=0, ZF=0, PF=0 */
    a = 5.0;
    b = 10.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 5.0 */
        "fldl %2\n\t"       /* ST(0) = 10.0, ST(1) = 5.0 */
        "fcomi %%st(1), %%st(0)\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "fstp %%st(0)\n\t"
        "fstp %%st(0)"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "fcomi 10>5: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "fcomi 10>5: ZF=0");
    TEST_ASSERT(!(flags & PF_FLAG), "fcomi 10>5: PF=0");

    /* ST(0) < ST(1): CF=1 */
    a = 10.0;
    b = 5.0;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 10.0 */
        "fldl %2\n\t"       /* ST(0) = 5.0, ST(1) = 10.0 */
        "fcomi %%st(1), %%st(0)\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "fstp %%st(0)\n\t"
        "fstp %%st(0)"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "fcomi 5<10: CF=1");
    TEST_ASSERT(!(flags & ZF_FLAG), "fcomi 5<10: ZF=0");

    /* ST(0) == ST(1): ZF=1 */
    a = 7.0;
    b = 7.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fcomi %%st(1), %%st(0)\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "fstp %%st(0)\n\t"
        "fstp %%st(0)"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "fcomi 7==7: ZF=1");
    TEST_ASSERT(!(flags & CF_FLAG), "fcomi 7==7: CF=0");
}

static void test_fcomip(void) {
    double a = 5.0, b = 10.0;
    uint64_t flags;

    /* FCOMIP: like FCOMI but pops ST(0) */
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 5.0 */
        "fldl %2\n\t"       /* ST(0) = 10.0, ST(1) = 5.0 */
        "fcomip %%st(1), %%st(0)\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "fstp %%st(0)"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "fcomip 10>5: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "fcomip 10>5: ZF=0");
}

static void test_fucomi(void) {
    double a, b;
    uint64_t flags;

    /* FUCOMI: unordered compare, sets PF=1 for NaN */
    a = 1.0;
    b = NAN;
    __asm__ volatile (
        "fldl %1\n\t"       /* ST(0) = 1.0 */
        "fldl %2\n\t"       /* ST(0) = NaN, ST(1) = 1.0 */
        "fucomi %%st(1), %%st(0)\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "fstp %%st(0)\n\t"
        "fstp %%st(0)"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "cc"
    );
    TEST_ASSERT(flags & PF_FLAG, "fucomi NaN: PF=1 (unordered)");
    TEST_ASSERT(flags & CF_FLAG, "fucomi NaN: CF=1 (unordered)");
    TEST_ASSERT(flags & ZF_FLAG, "fucomi NaN: ZF=1 (unordered)");

    /* Normal compare (no NaN) */
    a = 3.0;
    b = 5.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fucomi %%st(1), %%st(0)\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "fstp %%st(0)\n\t"
        "fstp %%st(0)"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "fucomi 5>3: CF=0");
    TEST_ASSERT(!(flags & PF_FLAG), "fucomi 5>3: PF=0");
}

static void test_fucomip(void) {
    double a = 1.0, b = NAN;
    uint64_t flags;

    __asm__ volatile (
        "fldl %1\n\t"
        "fldl %2\n\t"
        "fucomip %%st(1), %%st(0)\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "fstp %%st(0)"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "cc"
    );
    TEST_ASSERT(flags & PF_FLAG, "fucomip NaN: PF=1");
    TEST_ASSERT(flags & CF_FLAG, "fucomip NaN: CF=1");
    TEST_ASSERT(flags & ZF_FLAG, "fucomip NaN: ZF=1");
}

static void test_ftst(void) {
    double val;
    uint16_t sw;

    /* Positive: C3=0, C0=0 */
    val = 5.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "ftst\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    TEST_ASSERT(!(sw & 0x4500), "ftst +5: positive (C3=C2=C0=0)");

    /* Zero: C3=1 */
    val = 0.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "ftst\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x4000, "ftst 0: zero (C3=1)");

    /* Negative: C0=1 */
    val = -3.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "ftst\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x0100, "ftst -3: negative (C0=1)");

    /* NaN: unordered */
    val = NAN;
    __asm__ volatile (
        "fldl %1\n\t"
        "ftst\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x4500, "ftst NaN: unordered");

    /* +Inf */
    val = INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "ftst\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    TEST_ASSERT(!(sw & 0x4500), "ftst +inf: positive");

    /* -Inf */
    val = -INFINITY;
    __asm__ volatile (
        "fldl %1\n\t"
        "ftst\n\t"
        "fnstsw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "fstp %%st(0)"
        : "=m"(sw)
        : "m"(val)
        : "ax"
    );
    TEST_ASSERT((sw & 0x4500) == 0x0100, "ftst -inf: negative (C0=1)");
}

static void test_x87_compare_nan_exceptions_and_eflags_clear(void) {
    uint64_t qnan_bits = UINT64_C(0x7ff8123456789abc);
    double qnan, one = 1.0;
    memcpy(&qnan, &qnan_bits, sizeof(qnan));
    uint64_t flags;
    uint16_t status;

#define RUN_X87_COMI(INSN) do {                                             \
        __asm__ volatile (                                                  \
            "fldl %2\n\tfldl %3\n\tfnclex\n\t"                         \
            "movq $0x8d5, %%r11\n\tpushq %%r11\n\tpopfq\n\t"           \
            INSN " %%st(1), %%st(0)\n\tpushfq\n\tpopq %0\n\t"           \
            "fnstsw %1\n\tfstp %%st(0)\n\tfstp %%st(0)"                    \
            : "=r"(flags), "=m"(status) : "m"(one), "m"(qnan)          \
            : "r11", "cc", "memory");                                  \
    } while (0)

    RUN_X87_COMI("fucomi");
    TEST_ASSERT((flags & (ZF_FLAG | PF_FLAG | CF_FLAG)) ==
                (ZF_FLAG | PF_FLAG | CF_FLAG), "fucomi QNaN unordered flags");
    TEST_ASSERT(!(flags & (OF_FLAG | SF_FLAG | AF_FLAG)),
                "fucomi clears OF/SF/AF");
    TEST_ASSERT(!(status & 1), "fucomi QNaN does not set invalid status");

    RUN_X87_COMI("fcomi");
    TEST_ASSERT((flags & (ZF_FLAG | PF_FLAG | CF_FLAG)) ==
                (ZF_FLAG | PF_FLAG | CF_FLAG), "fcomi masked QNaN unordered flags");
    TEST_ASSERT(!(flags & (OF_FLAG | SF_FLAG | AF_FLAG)),
                "fcomi clears OF/SF/AF");
    TEST_ASSERT(status & 1, "fcomi QNaN sets invalid status");
#undef RUN_X87_COMI

    /* Memory FCOM also signals on a QNaN and writes unordered C bits. */
    __asm__ volatile (
        "fldl %1\n\tfnclex\n\tfcoml %2\n\tfnstsw %0\n\tfstp %%st(0)"
        : "=m"(status) : "m"(one), "m"(qnan) : "memory"
    );
    TEST_ASSERT((status & 0x4500) == 0x4500, "fcom memory QNaN unordered C bits");
    TEST_ASSERT(status & 1, "fcom memory QNaN sets invalid status");
    __asm__ volatile("fnclex");
}

int main(void) {
    TEST_START("FCOM/FCOMP/FCOMPP/FCOMI/FCOMIP/FUCOMI/FUCOMIP/FTST instructions");
    test_fcom();
    test_fcomp();
    test_fcompp();
    test_fcomi();
    test_fcomip();
    test_fucomi();
    test_fucomip();
    test_ftst();
    test_x87_compare_nan_exceptions_and_eflags_clear();
    TEST_END();
}
