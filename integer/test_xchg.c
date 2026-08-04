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

static void test_xchg_memory_widths_and_flags(void) {
    uint8_t m8 = UINT8_C(0x11), r8 = UINT8_C(0xaa);
    uint16_t m16 = UINT16_C(0x2233), r16 = UINT16_C(0xbbcc);
    uint32_t m32 = UINT32_C(0x44556677), r32 = UINT32_C(0xddeeff00);
    __asm__ volatile ("xchgb %0, %1" : "+q"(r8), "+m"(m8) : : "memory");
    __asm__ volatile ("xchgw %0, %1" : "+r"(r16), "+m"(m16) : : "memory");
    __asm__ volatile ("xchgl %0, %1" : "+r"(r32), "+m"(m32) : : "memory");
    TEST_ASSERT(r8 == UINT8_C(0x11) && m8 == UINT8_C(0xaa), "xchgb register/memory boundary");
    TEST_ASSERT(r16 == UINT16_C(0x2233) && m16 == UINT16_C(0xbbcc), "xchgw register/memory boundary");
    TEST_ASSERT(r32 == UINT32_C(0x44556677) && m32 == UINT32_C(0xddeeff00), "xchgl register/memory boundary");

    uint64_t before, after, reg = UINT64_C(0xffffffffffffffff);
    uint64_t mem = 0;
    __asm__ volatile (
        "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "pushfq\n\t" "popq %0\n\t" "xchgq %2, %3\n\t"
        "pushfq\n\t" "popq %1"
        : "=&r"(before), "=&r"(after), "+r"(reg), "+m"(mem)
        :
        : "r11", "cc", "memory");
    uint64_t flags = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((before & flags) == (after & flags), "xchg preserves status flags");
    TEST_ASSERT(reg == 0 && mem == UINT64_MAX, "xchg flags test exchanges zero/all-ones");
}

int main(void) {
    TEST_START("XCHG instruction");
    test_xchg_reg_reg();
    test_xchg_sizes();
    test_xchg_reg_mem();
    test_xchg_same_reg();
    test_xchg_memory_widths_and_flags();
    TEST_END();
}
