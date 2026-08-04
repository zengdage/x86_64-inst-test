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

static void test_and_width_memory_immediate_and_flags(void) {
    uint8_t m8 = UINT8_MAX;
    uint16_t m16 = UINT16_MAX;
    uint32_t m32 = UINT32_MAX;
    uint64_t m64 = UINT64_MAX;
    uint64_t result, flags;

    __asm__ volatile ("andb $0x80, %0" : "+m"(m8) : : "cc");
    __asm__ volatile ("andw $0x8001, %0" : "+m"(m16) : : "cc");
    __asm__ volatile ("andl $0x80000001, %0" : "+m"(m32) : : "cc");
    __asm__ volatile (
        "andq $-2147483648, %0\n\t"
        "pushfq\n\tpopq %1"
        : "+m"(m64), "=r"(flags) : : "cc"
    );
    TEST_ASSERT(m8 == UINT8_C(0x80), "andb memory boundary: %#x", m8);
    TEST_ASSERT(m16 == UINT16_C(0x8001), "andw memory boundary: %#x", m16);
    TEST_ASSERT(m32 == UINT32_C(0x80000001), "andl memory boundary: %#x", m32);
    TEST_ASSERT(m64 == UINT64_C(0xffffffff80000000),
                "andq sign-extended imm32 boundary: %#" PRIx64, m64);
    TEST_ASSERT(flags & SF_FLAG, "andq negative result: SF set");
    TEST_ASSERT(!(flags & ZF_FLAG), "andq nonzero result: ZF clear");
    TEST_ASSERT(flags & PF_FLAG, "andq low byte zero: PF set");
    TEST_ASSERT(!(flags & CF_FLAG), "andq clears CF");
    TEST_ASSERT(!(flags & OF_FLAG), "andq clears OF");

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "andl $0x80000000, %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result) : : "rax", "cc"
    );
    TEST_ASSERT(result == UINT64_C(0x0000000080000000),
                "andl writes eax and clears RAX high half: %#" PRIx64, result);
}

int main(void) {
    TEST_START("AND instruction");
    test_and_reg_reg();
    test_and_reg_imm();
    test_and_reg_mem();
    test_and_width_memory_immediate_and_flags();
    TEST_END();
}
