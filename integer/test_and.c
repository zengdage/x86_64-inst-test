/*
 * test_and.c - Test x86-64 AND instruction
 *
 * AND performs bitwise AND: DEST = DEST & SRC
 * Affects flags: SF, ZF, PF (CF and OF cleared, AF undefined)
 *
 * Compile: gcc -o test_and integer/test_and.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_and_reg_reg(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $0xFF00FF00FF00FF00, %%rax\n\t"
        "movq $0x00FF00FF00FF00FF, %%rbx\n\t"
        "andq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "andq: alternating masks expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "andq result=0: ZF should be set");
    TEST_ASSERT(!(flags & CF_FLAG), "andq: CF should be cleared");
    TEST_ASSERT(!(flags & OF_FLAG), "andq: OF should be cleared");

    __asm__ volatile (
        "movq $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
        "movq $0x123456789ABCDEF0, %%rbx\n\t"
        "andq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x123456789ABCDEF0UL, "andq with all-ones mask");
}

static void test_and_reg_imm(void) {
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "movl $0xFF, %%eax\n\t"
        "andl $0x0F, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 0x0F, "andl 0xFF & 0x0F expected 0x0F, got 0x%x", r32);

    /* 8-bit */
    uint8_t r8;
    __asm__ volatile (
        "movb $0xAB, %%al\n\t"
        "andb $0xF0, %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0xA0, "andb 0xAB & 0xF0 expected 0xA0, got 0x%x", r8);
    TEST_ASSERT(flags & SF_FLAG, "andb result=0xA0: SF should be set");
}

static void test_and_reg_mem(void) {
    uint64_t result;
    uint64_t mask = 0x00000000FFFFFFFFUL;

    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "andq %1, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        : "m"(mask)
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x9ABCDEF0UL, "andq reg,mem: low 32 bits");
}

int main(void) {
    TEST_START("AND instruction");
    test_and_reg_reg();
    test_and_reg_imm();
    test_and_reg_mem();
    TEST_END();
}
