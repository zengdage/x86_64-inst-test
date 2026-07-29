/*
 * test_push_pop.c - Test x86-64 PUSH/POP/PUSHFQ/POPFQ instructions
 *
 * PUSH decrements RSP and stores value.
 * POP loads value and increments RSP.
 * PUSHFQ/POPFQ push/pop the RFLAGS register.
 *
 * Compile: gcc -o test_push_pop integer/test_push_pop.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_push_pop_reg(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0xDEADBEEF, %%rax\n\t"
        "pushq %%rax\n\t"
        "popq %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 0xDEADBEEF, "push/pop reg: expected 0xDEADBEEF, got 0x%lx", result);
}

static void test_push_pop_imm(void) {
    uint64_t result;

    __asm__ volatile (
        "pushq $42\n\t"
        "popq %0"
        : "=r"(result)
    );
    TEST_ASSERT(result == 42, "push/pop imm: expected 42, got %lu", result);

    /* Negative immediate (sign extended) */
    __asm__ volatile (
        "pushq $-1\n\t"
        "popq %0"
        : "=r"(result)
    );
    TEST_ASSERT(result == 0xFFFFFFFFFFFFFFFFUL, "push/pop -1: sign extended to 64 bits");
}

static void test_push_pop_mem(void) {
    uint64_t val = 0x123456789ABCDEF0UL;
    uint64_t result;

    __asm__ volatile (
        "pushq %1\n\t"
        "popq %0"
        : "=r"(result)
        : "m"(val)
    );
    TEST_ASSERT(result == val, "push/pop mem");
}

static void test_push_rsp_changes(void) {
    uint64_t rsp_before, rsp_after;

    __asm__ volatile (
        "movq %%rsp, %0\n\t"
        "pushq $0\n\t"
        "movq %%rsp, %1\n\t"
        "addq $8, %%rsp"
        : "=r"(rsp_before), "=r"(rsp_after)
        :
        : "cc"
    );
    TEST_ASSERT(rsp_before - rsp_after == 8, "push decrements rsp by 8");
}

static void test_pushfq_popfq(void) {
    uint64_t flags_set, flags_read;

    /* Set CF, then verify via pushfq/popfq */
    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags_set)
        :
        : "cc"
    );
    TEST_ASSERT(flags_set & CF_FLAG, "pushfq after stc: CF should be set");

    /* Clear CF, verify */
    __asm__ volatile (
        "clc\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags_read)
        :
        : "cc"
    );
    TEST_ASSERT(!(flags_read & CF_FLAG), "pushfq after clc: CF should be clear");

    /* POPFQ to restore flags */
    __asm__ volatile (
        "clc\n\t"
        "pushfq\n\t"
        "popq %%rax\n\t"
        "orq %1, %%rax\n\t"    /* set CF bit */
        "pushq %%rax\n\t"
        "popfq\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags_read)
        : "i"(CF_FLAG)
        : "rax", "cc"
    );
    TEST_ASSERT(flags_read & CF_FLAG, "popfq: CF should be set after manual set");
}

static void test_push_pop_multiple(void) {
    uint64_t a, b, c;

    __asm__ volatile (
        "pushq $1\n\t"
        "pushq $2\n\t"
        "pushq $3\n\t"
        "popq %0\n\t"
        "popq %1\n\t"
        "popq %2"
        : "=r"(a), "=r"(b), "=r"(c)
    );
    /* Stack is LIFO */
    TEST_ASSERT(a == 3, "push/pop LIFO: first pop = 3, got %lu", a);
    TEST_ASSERT(b == 2, "push/pop LIFO: second pop = 2, got %lu", b);
    TEST_ASSERT(c == 1, "push/pop LIFO: third pop = 1, got %lu", c);
}

int main(void) {
    TEST_START("PUSH/POP/PUSHFQ/POPFQ instructions");
    test_push_pop_reg();
    test_push_pop_imm();
    test_push_pop_mem();
    test_push_rsp_changes();
    test_pushfq_popfq();
    test_push_pop_multiple();
    TEST_END();
}
