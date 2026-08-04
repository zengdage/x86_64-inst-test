/*
 * test_loop.c - Test x86-64 LOOP/LOOPE/LOOPNE instructions
 *
 * LOOP:   Decrements RCX, jumps if RCX != 0
 * LOOPE:  Decrements RCX, jumps if RCX != 0 AND ZF = 1
 * LOOPNE: Decrements RCX, jumps if RCX != 0 AND ZF = 0
 * These instructions do not affect flags (except RCX modification).
 *
 * Compile: gcc -o test_loop integer/test_loop.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_loop_basic(void) {
    uint64_t result;
    uint64_t rcx_final;

    /* Count from 5 to 0 using LOOP */
    __asm__ volatile (
        "xorq %0, %0\n\t"
        "movq $5, %%rcx\n\t"
        "1:\n\t"
        "addq $1, %0\n\t"
        "loop 1b\n\t"
        "movq %%rcx, %1"
        : "=r"(result), "=r"(rcx_final)
        :
        : "rcx", "cc"
    );
    TEST_ASSERT(result == 5, "loop 5 iterations: expected 5, got %lu", result);
    TEST_ASSERT(rcx_final == 0, "loop: rcx should be 0 after, got %lu", rcx_final);
}

static void test_loop_one(void) {
    uint64_t result = 0;

    /* LOOP with RCX=1 => executes body once, then exits */
    __asm__ volatile (
        "xorq %0, %0\n\t"
        "movq $1, %%rcx\n\t"
        "1:\n\t"
        "addq $1, %0\n\t"
        "loop 1b"
        : "=r"(result)
        :
        : "rcx", "cc"
    );
    TEST_ASSERT(result == 1, "loop with rcx=1: expected 1 iteration");
}

static void test_loop_zero_wrap(void) {
    uint64_t rcx_after;
    uint64_t took_branch;
    __asm__ volatile (
        "xorq %%rcx, %%rcx\n\t"
        "loop 1f\n\t"
        "movq $0, %0\n\t"
        "jmp 2f\n\t"
        "1: movq $1, %0\n\t"
        "2: movq %%rcx, %1"
        : "=&r"(took_branch), "=r"(rcx_after)
        :
        : "rcx", "cc");
    TEST_ASSERT(took_branch == 1, "loop with rcx=0 decrements and takes branch");
    TEST_ASSERT(rcx_after == UINT64_MAX, "loop with rcx=0 wraps to UINT64_MAX");
}

static void test_loop_flags_unchanged(void) {
    uint64_t before, after;
    __asm__ volatile (
        "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "movq $1, %%rcx\n\t" "pushfq\n\t" "popq %0\n\t"
        "loop 1f\n\t" "1: pushfq\n\t" "popq %1"
        : "=&r"(before), "=&r"(after)
        :
        : "rcx", "r11", "cc");
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((before & mask) == (after & mask),
                "loop preserves flags: before=%#" PRIx64 " after=%#" PRIx64,
                before & mask, after & mask);
}

static void test_loope_loopne_flag_matrix(void) {
    uint64_t branch;
#define TEST_CONDITIONAL_LOOP(SETFLAGS, INSN, EXPECTED, NAME) do { \
        __asm__ volatile ("movq $2, %%rcx\n\t" SETFLAGS "\n\t" INSN " 1f\n\t" \
                          "movq $0, %0\n\t" "jmp 2f\n\t" "1: movq $1, %0\n\t" "2:" \
                          : "=&r"(branch) : : "rax", "rcx", "cc"); \
        TEST_ASSERT(branch == (EXPECTED), NAME); \
    } while (0)
    TEST_CONDITIONAL_LOOP("xorl %%eax, %%eax", "loope",  1, "loope branches with rcx=2,ZF=1");
    TEST_CONDITIONAL_LOOP("movl $1, %%eax\n\ttestl %%eax, %%eax", "loope", 0, "loope stops with rcx=2,ZF=0");
    TEST_CONDITIONAL_LOOP("xorl %%eax, %%eax", "loopne", 0, "loopne stops with rcx=2,ZF=1");
    TEST_CONDITIONAL_LOOP("movl $1, %%eax\n\ttestl %%eax, %%eax", "loopne", 1, "loopne branches with rcx=2,ZF=0");
#undef TEST_CONDITIONAL_LOOP
}

static void test_loope(void) {
    uint64_t result;

    /* LOOPE: loop while equal (ZF=1) and RCX != 0 */
    /* Compare elements of two arrays until mismatch */
    uint64_t arr1[] = {1, 2, 3, 4, 5};
    uint64_t arr2[] = {1, 2, 3, 9, 5};  /* mismatch at index 3 */

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "leaq %2, %%rdi\n\t"
        "movq $5, %%rcx\n\t"
        "xorq %0, %0\n\t"
        "1:\n\t"
        "leaq 1(%0), %0\n\t"            /* increment without affecting flags */
        "movq (%%rsi), %%rax\n\t"
        "leaq 8(%%rsi), %%rsi\n\t"       /* advance without affecting flags */
        "cmpq (%%rdi), %%rax\n\t"        /* compare: sets ZF if equal */
        "leaq 8(%%rdi), %%rdi\n\t"       /* advance without affecting flags */
        "loope 1b"
        : "=&r"(result)
        : "m"(arr1), "m"(arr2)
        : "rax", "rcx", "rsi", "rdi", "cc"
    );
    /* Should stop at iteration 4 (index 3 mismatch) */
    TEST_ASSERT(result == 4, "loope: stopped at mismatch after %lu iterations", result);
}

static void test_loopne(void) {
    uint64_t result;

    /* LOOPNE: loop while not equal (ZF=0) and RCX != 0 */
    /* Search for value 42 in array */
    uint64_t arr[] = {1, 2, 42, 4, 5};

    __asm__ volatile (
        "leaq %1, %%rsi\n\t"
        "movq $5, %%rcx\n\t"
        "xorq %0, %0\n\t"
        "1:\n\t"
        "leaq 1(%0), %0\n\t"            /* increment without affecting flags */
        "cmpq $42, (%%rsi)\n\t"          /* compare: sets ZF if found */
        "leaq 8(%%rsi), %%rsi\n\t"       /* advance without affecting flags */
        "loopne 1b"
        : "=&r"(result)
        : "m"(arr)
        : "rcx", "rsi", "cc"
    );
    /* Should find 42 at index 2 (3rd iteration) */
    TEST_ASSERT(result == 3, "loopne: found value after %lu iterations", result);
}

int main(void) {
    TEST_START("LOOP/LOOPE/LOOPNE instructions");
    test_loop_basic();
    test_loop_one();
    test_loop_zero_wrap();
    test_loop_flags_unchanged();
    test_loope_loopne_flag_matrix();
    test_loope();
    test_loopne();
    TEST_END();
}
