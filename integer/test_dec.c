/*
 * test_dec.c - Test x86-64 DEC instruction
 *
 * DEC decrements operand by 1: DEST = DEST - 1
 * Affects flags: PF, AF, ZF, SF, OF (does NOT affect CF)
 *
 * Compile: gcc -o test_dec integer/test_dec.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_dec_basic(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "decq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "decq 1 expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "decq 1->0: ZF should be set");

    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "decq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0xFFFFFFFFFFFFFFFFUL, "decq 0 expected UINT64_MAX");
    TEST_ASSERT(flags & SF_FLAG, "decq 0->-1: SF should be set");
}

static void test_dec_overflow(void) {
    uint64_t result;
    uint64_t flags;

    /* Signed overflow: INT64_MIN - 1 */
    __asm__ volatile (
        "movabsq $0x8000000000000000, %%rax\n\t"
        "decq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x7FFFFFFFFFFFFFFFUL, "decq INT64_MIN expected INT64_MAX");
    TEST_ASSERT(flags & OF_FLAG, "decq INT64_MIN: OF should be set");
}

static void test_dec_cf_preserved(void) {
    uint64_t flags;

    __asm__ volatile (
        "stc\n\t"
        "movq $5, %%rax\n\t"
        "decq %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "decq: CF should be preserved (was set)");
}

static void test_dec_sizes(void) {
    uint8_t r8;
    uint16_t r16;
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "movb $0x00, %%al\n\t"
        "decb %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0xFF, "decb 0 expected 0xFF");

    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "decw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r16), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 0x7FFF, "decw 0x8000 expected 0x7FFF");
    TEST_ASSERT(flags & OF_FLAG, "decw 0x8000: OF should be set");

    __asm__ volatile (
        "movl $100, %%eax\n\t"
        "decl %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 99, "decl 100 expected 99");
}

int main(void) {
    TEST_START("DEC instruction");
    test_dec_basic();
    test_dec_overflow();
    test_dec_cf_preserved();
    test_dec_sizes();
    TEST_END();
}
