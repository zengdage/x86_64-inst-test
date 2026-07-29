/*
 * test_xchg.c - Test x86-64 XCHG instruction
 *
 * XCHG exchanges the contents of two operands atomically.
 * Does not affect any flags.
 *
 * Compile: gcc -o test_xchg integer/test_xchg.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_xchg_reg_reg(void) {
    uint64_t a, b;

    __asm__ volatile (
        "movq $0x1111, %%rax\n\t"
        "movq $0x2222, %%rbx\n\t"
        "xchgq %%rax, %%rbx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rbx, %1"
        : "=r"(a), "=r"(b)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(a == 0x2222 && b == 0x1111, "xchgq: a=%lu b=%lu", a, b);
}

static void test_xchg_sizes(void) {
    uint32_t a32, b32;
    uint16_t a16, b16;
    uint8_t a8, b8;

    __asm__ volatile (
        "movl $0xAAAA, %%eax\n\t"
        "movl $0x5555, %%ecx\n\t"
        "xchgl %%eax, %%ecx\n\t"
        "movl %%eax, %0\n\t"
        "movl %%ecx, %1"
        : "=r"(a32), "=r"(b32)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(a32 == 0x5555 && b32 == 0xAAAA, "xchgl");

    __asm__ volatile (
        "movw $0x1234, %%ax\n\t"
        "movw $0x5678, %%cx\n\t"
        "xchgw %%ax, %%cx\n\t"
        "movw %%ax, %0\n\t"
        "movw %%cx, %1"
        : "=r"(a16), "=r"(b16)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(a16 == 0x5678 && b16 == 0x1234, "xchgw");

    __asm__ volatile (
        "movb $0xAA, %%al\n\t"
        "movb $0x55, %%cl\n\t"
        "xchgb %%al, %%cl\n\t"
        "movb %%al, %0\n\t"
        "movb %%cl, %1"
        : "=r"(a8), "=r"(b8)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(a8 == 0x55 && b8 == 0xAA, "xchgb");
}

static void test_xchg_reg_mem(void) {
    uint64_t mem_val = 0xDEADBEEF;
    uint64_t reg_val;

    __asm__ volatile (
        "movq $0xCAFEBABE, %%rax\n\t"
        "xchgq %%rax, %1\n\t"
        "movq %%rax, %0"
        : "=r"(reg_val), "+m"(mem_val)
        :
        : "rax"
    );
    TEST_ASSERT(reg_val == 0xDEADBEEF, "xchgq reg,mem: reg got mem value");
    TEST_ASSERT(mem_val == 0xCAFEBABE, "xchgq reg,mem: mem got reg value");
}

static void test_xchg_same_reg(void) {
    uint64_t result;

    /* XCHG with same register => NOP */
    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "xchgq %%rax, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 42, "xchgq same reg: value unchanged");
}

int main(void) {
    TEST_START("XCHG instruction");
    test_xchg_reg_reg();
    test_xchg_sizes();
    test_xchg_reg_mem();
    test_xchg_same_reg();
    TEST_END();
}
