/*
 * test_not.c - Test x86-64 NOT instruction
 *
 * NOT performs bitwise complement: DEST = ~DEST
 * Does NOT affect any flags.
 *
 * Compile: gcc -o test_not integer/test_not.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_not_reg(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "notq %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0xFFFFFFFFFFFFFFFFUL, "notq 0 expected all ones");

    __asm__ volatile (
        "movq $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
        "notq %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0, "notq all-ones expected 0");

    __asm__ volatile (
        "movq $0xAAAAAAAAAAAAAAAA, %%rax\n\t"
        "notq %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0x5555555555555555UL, "notq 0xAA..AA expected 0x55..55");
}

static void test_not_sizes(void) {
    uint8_t r8;
    uint16_t r16;
    uint32_t r32;

    __asm__ volatile (
        "movb $0x0F, %%al\n\t"
        "notb %%al\n\t"
        "movb %%al, %0"
        : "=r"(r8)
        :
        : "rax"
    );
    TEST_ASSERT(r8 == 0xF0, "notb 0x0F expected 0xF0, got 0x%x", r8);

    __asm__ volatile (
        "movw $0x00FF, %%ax\n\t"
        "notw %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(r16)
        :
        : "rax"
    );
    TEST_ASSERT(r16 == 0xFF00, "notw 0x00FF expected 0xFF00, got 0x%x", r16);

    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "notl %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax"
    );
    TEST_ASSERT(r32 == 0xEDCBA987, "notl 0x12345678 expected 0xEDCBA987, got 0x%x", r32);
}

static void test_not_mem(void) {
    uint64_t val = 0x123456789ABCDEF0UL;
    __asm__ volatile (
        "notq %0"
        : "+m"(val)
    );
    TEST_ASSERT(val == 0xEDCBA98765432110UL - 1, "notq mem");
    /* Actually ~0x123456789ABCDEF0 = 0xEDCBA9876543210F */
    val = 0x123456789ABCDEF0UL;
    __asm__ volatile (
        "notq %0"
        : "+m"(val)
    );
    TEST_ASSERT(val == 0xEDCBA9876543210FUL, "notq mem: correct complement");
}

static void test_not_no_flags(void) {
    uint64_t flags_before, flags_after;

    /* NOT should not change flags */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"    /* set ZF */
        "pushfq\n\t"
        "popq %0\n\t"
        "notq %%rax\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        :
        : "rax", "cc"
    );
    /* Check that status flags are unchanged by NOT */
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "NOT should not change flags: before=0x%lx after=0x%lx",
                flags_before & mask, flags_after & mask);
}

int main(void) {
    TEST_START("NOT instruction");
    test_not_reg();
    test_not_sizes();
    test_not_mem();
    test_not_no_flags();
    TEST_END();
}
