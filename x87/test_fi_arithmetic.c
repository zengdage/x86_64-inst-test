#include "../common.h"
#include <math.h>

#define RUN_FI16(NAME, INSN, ST0, INTEGER, EXPECTED) do {                     \
    const double st0 = (ST0);                                                  \
    const int16_t integer = (INTEGER);                                         \
    double result;                                                              \
    __asm__ volatile ("fldl %1\n\t" INSN " %2\n\tfstpl %0"                \
                      : "=m"(result) : "m"(st0), "m"(integer));              \
    TEST_ASSERT(result == (EXPECTED), NAME " m16int: got %.17g", result);     \
} while (0)

#define RUN_FI32(NAME, INSN, ST0, INTEGER, EXPECTED) do {                     \
    const double st0 = (ST0);                                                  \
    const int32_t integer = (INTEGER);                                         \
    double result;                                                              \
    __asm__ volatile ("fldl %1\n\t" INSN " %2\n\tfstpl %0"                \
                      : "=m"(result) : "m"(st0), "m"(integer));              \
    TEST_ASSERT(result == (EXPECTED), NAME " m32int: got %.17g", result);     \
} while (0)

static void test_fiadd_fimul(void) {
    RUN_FI16("fiadd", "fiadds", 20.5, -3, 17.5);
    RUN_FI32("fiadd", "fiaddl", -0.5, INT32_MAX, 2147483646.5);
    RUN_FI16("fimul", "fimuls", -7.5, -4, 30.0);
    RUN_FI32("fimul", "fimull", 0x1p-20, INT32_MIN, -2048.0);
}

static void test_fisub_fisubr(void) {
    RUN_FI16("fisub", "fisubs", 20.5, -3, 23.5);
    RUN_FI32("fisub", "fisubl", -0.5, INT32_MAX, -2147483647.5);
    RUN_FI16("fisubr", "fisubrs", 20.5, -3, -23.5);
    RUN_FI32("fisubr", "fisubrl", -0.5, INT32_MAX, 2147483647.5);
}

static void test_fidiv_fidivr(void) {
    RUN_FI16("fidiv", "fidivs", 21.0, -3, -7.0);
    RUN_FI32("fidiv", "fidivl", -2147483648.0, INT32_MIN, 1.0);
    RUN_FI16("fidivr", "fidivrs", 4.0, -10, -2.5);
    RUN_FI32("fidivr", "fidivrl", -0.5, INT32_MIN, 4294967296.0);
}

static void test_integer_divide_by_zero(void) {
    const double value = -1.0;
    const int32_t zero = 0;
    double result;
    uint16_t status;
    __asm__ volatile (
        "fnclex\n\tfldl %2\n\tfidivl %3\n\tfstpl %0\n\tfnstsw %1"
        : "=m"(result), "=m"(status) : "m"(value), "m"(zero) : "memory");
    TEST_ASSERT(isinf(result) && signbit(result), "fidiv by integer zero yields -infinity");
    TEST_ASSERT(status & (1u << 2), "fidiv by integer zero sets divide-by-zero status");
    __asm__ volatile ("fnclex");
}

int main(void) {
    TEST_START("x87 FIADD/FIMUL/FISUB/FISUBR/FIDIV/FIDIVR");
    test_fiadd_fimul();
    test_fisub_fisubr();
    test_fidiv_fidivr();
    test_integer_divide_by_zero();
    TEST_END();
}
