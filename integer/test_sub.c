/*
 * test_sub.c - Test x86-64 SUB instruction
 *
 * SUB performs integer subtraction: DEST = DEST - SRC
 * Affects flags: CF, PF, AF, ZF, SF, OF
 *
 * Compile: gcc -o test_sub integer/test_sub.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_sub_reg_reg(void) {
    uint64_t result;
    uint64_t flags;

    /* 64-bit: 5 - 3 = 2 */
    __asm__ volatile (
        "movq $5, %%rax\n\t"
        "movq $3, %%rbx\n\t"
        "subq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 2, "subq reg,reg: 5-3 expected 2, got %lu", result);
    TEST_ASSERT(!(flags & CF_FLAG), "subq 5-3: CF should be clear");
    TEST_ASSERT(!(flags & ZF_FLAG), "subq 5-3: ZF should be clear");

    /* 64-bit: 3 - 3 = 0, ZF set */
    __asm__ volatile (
        "movq $3, %%rax\n\t"
        "movq $3, %%rbx\n\t"
        "subq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "subq 3-3 expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "subq 3-3: ZF should be set");

    /* 64-bit: 0 - 1 => borrow, CF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "movq $1, %%rbx\n\t"
        "subq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0xFFFFFFFFFFFFFFFFUL, "subq 0-1 expected UINT64_MAX");
    TEST_ASSERT(flags & CF_FLAG, "subq 0-1: CF should be set (borrow)");
    TEST_ASSERT(flags & SF_FLAG, "subq 0-1: SF should be set");

    /* 64-bit: signed overflow: INT64_MIN - 1 */
    __asm__ volatile (
        "movabsq $0x8000000000000000, %%rax\n\t"
        "movq $1, %%rbx\n\t"
        "subq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x7FFFFFFFFFFFFFFFUL, "subq INT64_MIN-1 expected INT64_MAX");
    TEST_ASSERT(flags & OF_FLAG, "subq INT64_MIN-1: OF should be set");
}

static void test_sub_reg_imm(void) {
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "movl $300, %%eax\n\t"
        "subl $200, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 100, "subl imm: 300-200 expected 100, got %u", r32);

    /* 8-bit borrow */
    uint8_t r8;
    __asm__ volatile (
        "movb $0x00, %%al\n\t"
        "subb $1, %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0xFF, "subb 0-1 expected 0xFF, got 0x%x", r8);
    TEST_ASSERT(flags & CF_FLAG, "subb 0-1: CF should be set");
}

static void test_sub_reg_mem(void) {
    uint64_t result;
    uint64_t mem_val = 30;

    __asm__ volatile (
        "movq $100, %%rax\n\t"
        "subq %1, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        : "m"(mem_val)
        : "rax", "cc"
    );
    TEST_ASSERT(result == 70, "subq reg,mem: 100-30 expected 70, got %lu", result);
}

int main(void) {
    TEST_START("SUB instruction");
    test_sub_reg_reg();
    test_sub_reg_imm();
    test_sub_reg_mem();
    TEST_END();
}
