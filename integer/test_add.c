/*
 * test_add.c - Test x86-64 ADD instruction
 *
 * ADD performs integer addition: DEST = DEST + SRC
 * Affects flags: CF, PF, AF, ZF, SF, OF
 *
 * Compile: gcc -o test_add integer/test_add.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_add_reg_reg(void) {
    uint64_t result;
    uint64_t flags;

    /* 64-bit: 1 + 2 = 3 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "movq $2, %%rbx\n\t"
        "addq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 3, "addq reg,reg: 1+2 expected 3, got %lu", result);
    TEST_ASSERT(!(flags & ZF_FLAG), "addq 1+2: ZF should be clear");
    TEST_ASSERT(!(flags & SF_FLAG), "addq 1+2: SF should be clear");
    TEST_ASSERT(!(flags & CF_FLAG), "addq 1+2: CF should be clear");
    TEST_ASSERT(!(flags & OF_FLAG), "addq 1+2: OF should be clear");

    /* 64-bit: 0 + 0 = 0, ZF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "xorq %%rbx, %%rbx\n\t"
        "addq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "addq reg,reg: 0+0 expected 0, got %lu", result);
    TEST_ASSERT(flags & ZF_FLAG, "addq 0+0: ZF should be set");

    /* 64-bit: overflow - max signed + 1 => OF set */
    __asm__ volatile (
        "movabsq $0x7FFFFFFFFFFFFFFF, %%rax\n\t"
        "movq $1, %%rbx\n\t"
        "addq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x8000000000000000UL, "addq LLONG_MAX+1 expected 0x8000000000000000");
    TEST_ASSERT(flags & OF_FLAG, "addq LLONG_MAX+1: OF should be set");
    TEST_ASSERT(flags & SF_FLAG, "addq LLONG_MAX+1: SF should be set");

    /* 64-bit: carry - UINT64_MAX + 1 => CF set, result 0 */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "movq $1, %%rbx\n\t"
        "addq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "addq UINT64_MAX+1 expected 0");
    TEST_ASSERT(flags & CF_FLAG, "addq UINT64_MAX+1: CF should be set");
    TEST_ASSERT(flags & ZF_FLAG, "addq UINT64_MAX+1: ZF should be set");
}

static void test_add_reg_imm(void) {
    uint64_t flags;

    /* 32-bit add immediate */
    uint32_t r32;
    __asm__ volatile (
        "movl $100, %%eax\n\t"
        "addl $200, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 300, "addl imm: 100+200 expected 300, got %u", r32);

    /* 8-bit add immediate, SF test */
    uint8_t r8;
    __asm__ volatile (
        "movb $0x7F, %%al\n\t"
        "addb $1, %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0x80, "addb 0x7F+1 expected 0x80, got 0x%x", r8);
    TEST_ASSERT(flags & OF_FLAG, "addb 0x7F+1: OF should be set");
    TEST_ASSERT(flags & SF_FLAG, "addb 0x7F+1: SF should be set");

    /* 16-bit add immediate */
    uint16_t r16;
    __asm__ volatile (
        "movw $0xFFFF, %%ax\n\t"
        "addw $1, %%ax\n\t"
        "movw %%ax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r16), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 0, "addw 0xFFFF+1 expected 0, got 0x%x", r16);
    TEST_ASSERT(flags & CF_FLAG, "addw 0xFFFF+1: CF should be set");
    TEST_ASSERT(flags & ZF_FLAG, "addw 0xFFFF+1: ZF should be set");
}

static void test_add_reg_mem(void) {
    uint64_t result;
    uint64_t flags;
    uint64_t mem_val = 42;

    __asm__ volatile (
        "movq $58, %%rax\n\t"
        "addq %2, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        : "m"(mem_val)
        : "rax", "cc"
    );
    TEST_ASSERT(result == 100, "addq reg,mem: 58+42 expected 100, got %lu", result);
}

static void test_add_pf_af(void) {
    uint64_t flags;

    /* PF: result byte has even number of 1-bits */
    __asm__ volatile (
        "movb $0x01, %%al\n\t"
        "addb $0x02, %%al\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    /* 0x03 = 0b00000011 => 2 set bits => PF set */
    TEST_ASSERT(flags & PF_FLAG, "addb 1+2=3: PF should be set (even parity)");

    /* AF: carry from bit 3 to bit 4 */
    __asm__ volatile (
        "movb $0x0F, %%al\n\t"
        "addb $0x01, %%al\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & AF_FLAG, "addb 0x0F+1: AF should be set");
}

int main(void) {
    TEST_START("ADD instruction");
    test_add_reg_reg();
    test_add_reg_imm();
    test_add_reg_mem();
    test_add_pf_af();
    TEST_END();
}
