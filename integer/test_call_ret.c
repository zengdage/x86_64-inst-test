/*
 * test_call_ret.c - Test x86-64 CALL/RET instructions
 *
 * CALL pushes return address and jumps to target.
 * RET pops return address and jumps to it.
 *
 * Compile: gcc -o test_call_ret integer/test_call_ret.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_call_ret_direct(void) {
    uint64_t result;

    __asm__ volatile (
        "call 1f\n\t"
        "jmp 2f\n\t"
        "1:\n\t"
        "movq $42, %0\n\t"
        "ret\n\t"
        "2:"
        : "=r"(result)
        :
        : "cc"
    );
    TEST_ASSERT(result == 42, "call/ret direct: expected 42, got %lu", result);
}

static void test_call_indirect(void) {
    uint64_t result;

    __asm__ volatile (
        "leaq 1f(%%rip), %%rax\n\t"
        "call *%%rax\n\t"
        "jmp 2f\n\t"
        "1:\n\t"
        "movq $99, %0\n\t"
        "ret\n\t"
        "2:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 99, "call indirect: expected 99, got %lu", result);
}

static void test_call_pushes_return_addr(void) {
    uint64_t rsp_before, rsp_after;

    __asm__ volatile (
        "movq %%rsp, %0\n\t"
        "call 1f\n\t"
        "jmp 2f\n\t"
        "1:\n\t"
        "movq %%rsp, %1\n\t"
        "addq $8, %%rsp\n\t"   /* pop return address manually */
        "2:"
        : "=r"(rsp_before), "=r"(rsp_after)
        :
        : "cc"
    );
    TEST_ASSERT(rsp_before - rsp_after == 8,
                "call pushes 8 bytes: rsp diff = %ld", (long)(rsp_before - rsp_after));
}

static void test_nested_call(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0, %0\n\t"
        "call 1f\n\t"
        "jmp 3f\n\t"
        "1:\n\t"
        "addq $1, %0\n\t"
        "call 2f\n\t"
        "ret\n\t"
        "2:\n\t"
        "addq $10, %0\n\t"
        "ret\n\t"
        "3:"
        : "=r"(result)
        :
        : "cc"
    );
    TEST_ASSERT(result == 11, "nested call: expected 11, got %lu", result);
}

int main(void) {
    TEST_START("CALL/RET instructions");
    test_call_ret_direct();
    test_call_indirect();
    test_call_pushes_return_addr();
    test_nested_call();
    TEST_END();
}
