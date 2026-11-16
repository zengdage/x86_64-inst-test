#include "../common.h"

#define CHECK_FCMOV(NAME, SET_FLAGS, INSN, EXPECTED) do {                      \
    const double source = 91.25, destination = -7.5;                           \
    double result;                                                              \
    __asm__ volatile (                                                          \
        "fldl %[source]\n\t"                                                  \
        "fldl %[destination]\n\t"                                             \
        SET_FLAGS "\n\t" INSN " %%st(1), %%st\n\t"                           \
        "fstpl %[result]\n\t"                                                 \
        "fstp %%st(0)"                                                         \
        : [result] "=m"(result)                                                \
        : [source] "m"(source), [destination] "m"(destination)                \
        : "rax", "cc");                                                       \
    TEST_ASSERT(result == (EXPECTED), NAME ": got %.17g", result);            \
} while (0)

static void test_positive_conditions(void) {
    CHECK_FCMOV("fcmovb true",  "stc", "fcmovb", 91.25);
    CHECK_FCMOV("fcmovb false", "clc", "fcmovb", -7.5);
    CHECK_FCMOV("fcmove true",  "xorl %%eax, %%eax", "fcmove", 91.25);
    CHECK_FCMOV("fcmove false", "movl $1, %%eax\n\ttestl %%eax, %%eax", "fcmove", -7.5);
    CHECK_FCMOV("fcmovbe true by CF", "stc", "fcmovbe", 91.25);
    CHECK_FCMOV("fcmovbe false", "movl $1, %%eax\n\ttestl %%eax, %%eax", "fcmovbe", -7.5);
    CHECK_FCMOV("fcmovu true",  "xorl %%eax, %%eax", "fcmovu", 91.25);
    CHECK_FCMOV("fcmovu false", "movl $1, %%eax\n\ttestl %%eax, %%eax", "fcmovu", -7.5);
}

static void test_negative_conditions(void) {
    CHECK_FCMOV("fcmovnb true",  "clc", "fcmovnb", 91.25);
    CHECK_FCMOV("fcmovnb false", "stc", "fcmovnb", -7.5);
    CHECK_FCMOV("fcmovne true",  "movl $1, %%eax\n\ttestl %%eax, %%eax", "fcmovne", 91.25);
    CHECK_FCMOV("fcmovne false", "xorl %%eax, %%eax", "fcmovne", -7.5);
    CHECK_FCMOV("fcmovnbe true", "movl $1, %%eax\n\ttestl %%eax, %%eax", "fcmovnbe", 91.25);
    CHECK_FCMOV("fcmovnbe false by CF", "stc", "fcmovnbe", -7.5);
    CHECK_FCMOV("fcmovnu true",  "movl $1, %%eax\n\ttestl %%eax, %%eax", "fcmovnu", 91.25);
    CHECK_FCMOV("fcmovnu false", "xorl %%eax, %%eax", "fcmovnu", -7.5);
}

int main(void) {
    TEST_START("x87 FCMOVcc instructions");
    test_positive_conditions();
    test_negative_conditions();
    TEST_END();
}
