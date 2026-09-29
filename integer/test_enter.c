/*
 * test_enter.c - Test ENTER nested frame traversal
 *
 * Compile: gcc -o test_enter integer/test_enter.c -O2
 * Note: Do not use static linking.
 */
#include "../common.h"

typedef struct {
    uint64_t rbp;
    uint64_t rsp;
    uint64_t stack[5];
} enter_result_t;

extern void run_enter_level4(uint64_t *stack_top, uint64_t *old_rbp,
                             enter_result_t *result);

__asm__(
    ".text\n"
    ".globl run_enter_level4\n"
    ".type run_enter_level4, @function\n"
    "run_enter_level4:\n\t"
    "movq %rsp, %r8\n\t"
    "movq %rbp, %r9\n\t"
    "movq %rdi, %rsp\n\t"
    "movq %rsi, %rbp\n\t"
    "enter $0, $4\n\t"
    "movq %rbp, 0(%rdx)\n\t"
    "movq %rsp, 8(%rdx)\n\t"
    "movq 0(%rsp), %rax\n\t"
    "movq %rax, 16(%rdx)\n\t"
    "movq 8(%rsp), %rax\n\t"
    "movq %rax, 24(%rdx)\n\t"
    "movq 16(%rsp), %rax\n\t"
    "movq %rax, 32(%rdx)\n\t"
    "movq 24(%rsp), %rax\n\t"
    "movq %rax, 40(%rdx)\n\t"
    "movq 32(%rsp), %rax\n\t"
    "movq %rax, 48(%rdx)\n\t"
    "movq %r8, %rsp\n\t"
    "movq %r9, %rbp\n\t"
    "ret\n"
    ".size run_enter_level4, .-run_enter_level4\n"
);

static void test_nested_enter_traversal(void) {
    TEST_START("ENTER nested frame traversal");

    uint64_t frame_chain[8] = {0};
    uint64_t enter_stack[16] = {0};
    enter_result_t result = {0};
    uint64_t *old_rbp = &frame_chain[4];
    uint64_t *stack_top = &enter_stack[16];

    frame_chain[3] = UINT64_C(0x1111111122222222);
    frame_chain[2] = UINT64_C(0x3333333344444444);
    frame_chain[1] = UINT64_C(0x5555555566666666);

    run_enter_level4(stack_top, old_rbp, &result);

    TEST_ASSERT(result.rbp == (uint64_t)(uintptr_t)(stack_top - 1),
                "ENTER frame pointer: %#" PRIx64, result.rbp);
    TEST_ASSERT(result.rsp == (uint64_t)(uintptr_t)(stack_top - 5),
                "ENTER stack pointer: %#" PRIx64, result.rsp);

    /* Final frame link, followed by the three traversed parent-frame slots. */
    TEST_ASSERT(result.stack[0] == result.rbp,
                "ENTER final frame link: %#" PRIx64, result.stack[0]);
    TEST_ASSERT(result.stack[1] == frame_chain[1],
                "ENTER third parent slot: %#" PRIx64, result.stack[1]);
    TEST_ASSERT(result.stack[2] == frame_chain[2],
                "ENTER second parent slot: %#" PRIx64, result.stack[2]);
    TEST_ASSERT(result.stack[3] == frame_chain[3],
                "ENTER first parent slot: %#" PRIx64, result.stack[3]);
    TEST_ASSERT(result.stack[4] == (uint64_t)(uintptr_t)old_rbp,
                "ENTER saved RBP: %#" PRIx64, result.stack[4]);
}

int main(void) {
    test_nested_enter_traversal();
    TEST_END();
}
