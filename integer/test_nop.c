/*
 * test_nop.c - Test x86-64 NOP instruction
 *
 * NOP performs no operation. It does not affect any registers or flags.
 * Multi-byte NOP (0x0F 0x1F ...) is also available for alignment padding.
 *
 * Compile: gcc -o test_nop integer/test_nop.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_nop_basic(void) {
    uint64_t before, after;

    /* NOP should not change register values */
    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "nop\n\t"
        "movq %%rax, %1"
        : "=r"(before), "=r"(after)
        :
        : "rax"
    );
    TEST_ASSERT(before == after, "nop: register unchanged");
}

static void test_nop_no_flags(void) {
    uint64_t flags_before, flags_after;

    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        :
        : "cc"
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask), "nop: flags unchanged");
}

static void test_nop_multi_byte(void) {
    uint64_t result;

    /* Multi-byte NOP forms (for alignment) */
    __asm__ volatile (
        "movq $99, %%rax\n\t"
        ".byte 0x0F, 0x1F, 0x00\n\t"           /* 3-byte NOP */
        ".byte 0x0F, 0x1F, 0x40, 0x00\n\t"     /* 4-byte NOP */
        ".byte 0x0F, 0x1F, 0x44, 0x00, 0x00\n\t" /* 5-byte NOP */
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 99, "multi-byte nop: register unchanged");
}

static void test_nop_sequence(void) {
    uint64_t result;

    /* Many NOPs in sequence */
    __asm__ volatile (
        "movq $12345, %%rax\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 12345, "16 nops: register unchanged");
}

int main(void) {
    TEST_START("NOP instruction");
    test_nop_basic();
    test_nop_no_flags();
    test_nop_multi_byte();
    test_nop_sequence();
    TEST_END();
}
