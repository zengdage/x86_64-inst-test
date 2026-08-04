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

static void test_ret_imm16_stack_adjust(void) {
    uint64_t rsp_before, rsp_after, result;
    __asm__ volatile (
        "movq %%rsp, %0\n\t" "subq $16, %%rsp\n\t" "call 1f\n\t"
        "movq %%rsp, %1\n\t" "jmp 2f\n\t"
        "1: movq $123, %2\n\t" "ret $16\n\t" "2:"
        : "=&r"(rsp_before), "=&r"(rsp_after), "=&r"(result)
        :
        : "cc", "memory");
    TEST_ASSERT(result == 123, "ret imm16 returns to caller");
    TEST_ASSERT(rsp_after == rsp_before, "ret imm16 discards 16 argument bytes");
}

static void test_call_memory_indirect(void) {
    void *target;
    uint64_t result;
    __asm__ volatile (
        "leaq 1f(%%rip), %%rax\n\t" "movq %%rax, %1\n\t"
        "call *%1\n\t" "jmp 2f\n\t"
        "1: movq $321, %0\n\t" "ret\n\t" "2:"
        : "=&r"(result), "=m"(target)
        :
        : "rax", "cc", "memory");
    TEST_ASSERT(result == 321, "call through memory operand");
}

int main(void) {
    TEST_START("CALL/RET instructions");
    test_call_ret_direct();
    test_call_indirect();
    test_call_pushes_return_addr();
    test_nested_call();
    test_ret_imm16_stack_adjust();
    test_call_memory_indirect();
    TEST_END();
}
