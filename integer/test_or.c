/*
 * test_or.c - Test x86-64 OR instruction
 *
 * OR performs bitwise OR: DEST = DEST | SRC
 * Affects flags: SF, ZF, PF (CF and OF cleared, AF undefined)
 *
 * Compile: gcc -o test_or integer/test_or.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_or_reg_reg(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $0xFF00FF00, %%rax\n\t"
        "movq $0x00FF00FF, %%rbx\n\t"
        "orq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0xFFFFFFFFUL, "orq: combined = 0xFFFFFFFF");
    TEST_ASSERT(!(flags & CF_FLAG), "orq: CF should be cleared");
    TEST_ASSERT(!(flags & OF_FLAG), "orq: OF should be cleared");

    /* OR with 0 => unchanged */
    __asm__ volatile (
        "movq $0x1234, %%rax\n\t"
        "xorq %%rbx, %%rbx\n\t"
        "orq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x1234, "orq with 0: unchanged");

    /* OR 0 with 0 => ZF */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "orq $0, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "orq 0|0: ZF should be set");
}

static void test_or_reg_imm(void) {
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "movl $0xF0, %%eax\n\t"
        "orl $0x0F, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 0xFF, "orl 0xF0|0x0F expected 0xFF, got 0x%x", r32);

    /* SF test */
    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "orl $0, %%eax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & SF_FLAG, "orl 0x80000000: SF should be set");
}

static void test_or_sizes(void) {
    uint8_t r8;
    uint16_t r16;

    __asm__ volatile (
        "movb $0xA0, %%al\n\t"
        "orb $0x05, %%al\n\t"
        "movb %%al, %0"
        : "=r"(r8)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0xA5, "orb 0xA0|0x05 expected 0xA5");

    __asm__ volatile (
        "movw $0xFF00, %%ax\n\t"
        "orw $0x00FF, %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(r16)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 0xFFFF, "orw 0xFF00|0x00FF expected 0xFFFF");
}

int main(void) {
    TEST_START("OR instruction");
    test_or_reg_reg();
    test_or_reg_imm();
    test_or_sizes();
    TEST_END();
}
