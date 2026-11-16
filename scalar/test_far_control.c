#include "../common.h"

struct far_pointer64 {
    uint64_t offset;
    uint16_t selector;
} __attribute__((packed));

static void test_far_call_ret(void) {
    struct far_pointer64 pointer;
    uint64_t marker = 0, before_rsp, after_rsp;
    __asm__ volatile (
        "movq %%rsp, %[before_rsp]\n\t"
        "leaq 1f(%%rip), %%rax\n\tmovq %%rax, 0(%%rdi)\n\t"
        "movw %%cs, %%ax\n\tmovw %%ax, 8(%%rdi)\n\t"
        ".byte 0x48, 0xff, 0x1f\n\t" /* CALL FAR m16:64 [RDI] */
        "jmp 2f\n\t"
        "1: movq $1, %[marker]\n\tlretq\n\t"
        "2: movq %%rsp, %[after_rsp]"
        : [marker] "+r"(marker), [before_rsp] "=&r"(before_rsp),
          [after_rsp] "=&r"(after_rsp), "+m"(pointer)
        : "D"(&pointer) : "rax", "cc", "memory");
    TEST_ASSERT(marker == 1, "far CALL reaches a target in the current code segment");
    TEST_ASSERT(after_rsp == before_rsp, "far RET restores RSP after far CALL");
}

static void test_far_jump(void) {
    struct far_pointer64 pointer;
    uint64_t marker = 0;
    __asm__ volatile (
        "leaq 1f(%%rip), %%rax\n\tmovq %%rax, 0(%%rdi)\n\t"
        "movw %%cs, %%ax\n\tmovw %%ax, 8(%%rdi)\n\t"
        ".byte 0x48, 0xff, 0x2f\n\t" /* JMP FAR m16:64 [RDI] */
        "ud2\n\t"
        "1: movq $1, %[marker]"
        : [marker] "+r"(marker), "+m"(pointer)
        : "D"(&pointer) : "rax", "cc", "memory");
    TEST_ASSERT(marker == 1, "far JMP reaches a target in the current code segment");
}

static void test_iretq(void) {
    uint64_t marker = 0, before_rsp, after_rsp, after_flags;
    __asm__ volatile (
        "movq %%rsp, %[before_rsp]\n\t"
        "xorq %%rax, %%rax\n\tmovw %%ss, %%ax\n\tpushq %%rax\n\t"
        "pushq %[before_rsp]\n\t"
        "pushfq\n\tpopq %%rax\n\torq $1, %%rax\n\tpushq %%rax\n\t"
        "xorq %%rax, %%rax\n\tmovw %%cs, %%ax\n\tpushq %%rax\n\t"
        "leaq 1f(%%rip), %%rax\n\tpushq %%rax\n\tiretq\n\t"
        "ud2\n\t"
        "1: pushfq\n\tpopq %[after_flags]\n\t"
        "movq %%rsp, %[after_rsp]\n\tmovq $1, %[marker]"
        : [marker] "+r"(marker), [before_rsp] "=&r"(before_rsp),
          [after_rsp] "=&r"(after_rsp), [after_flags] "=&r"(after_flags)
        : : "rax", "cc", "memory");
    TEST_ASSERT(marker == 1, "IRETQ resumes at the same-privilege target");
    TEST_ASSERT(after_rsp == before_rsp, "IRETQ consumes the RIP/CS/RFLAGS frame");
    TEST_ASSERT(after_flags & CF_FLAG, "IRETQ restores modifiable RFLAGS bits");
}

int main(void) {
    TEST_START("same-privilege far CALL/JMP/RET and IRETQ");
    test_far_call_ret();
    test_far_jump();
    test_iretq();
    TEST_END();
}
