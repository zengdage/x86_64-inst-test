/*
 * test_neg.c - Test x86-64 NEG instruction
 *
 * NEG performs two's complement negation: DEST = 0 - DEST
 * Affects flags: CF (set if operand != 0), PF, AF, ZF, SF, OF
 *
 * Compile: gcc -o test_neg integer/test_neg.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_neg_basic(void) {
    int64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "negq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == -1, "negq 1 expected -1");
    TEST_ASSERT(flags & CF_FLAG, "negq 1: CF should be set (operand != 0)");
    TEST_ASSERT(flags & SF_FLAG, "negq 1: SF should be set");

    __asm__ volatile (
        "movq $-42, %%rax\n\t"
        "negq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 42, "negq -42 expected 42");
}

static void test_neg_zero(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "negq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "negq 0 expected 0");
    TEST_ASSERT(!(flags & CF_FLAG), "negq 0: CF should be clear");
    TEST_ASSERT(flags & ZF_FLAG, "negq 0: ZF should be set");
}

static void test_neg_min(void) {
    int64_t result;
    uint64_t flags;

    /* NEG of INT64_MIN => overflow (result is INT64_MIN again) */
    __asm__ volatile (
        "movabsq $0x8000000000000000, %%rax\n\t"
        "negq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT((uint64_t)result == 0x8000000000000000UL, "negq INT64_MIN: result is INT64_MIN");
    TEST_ASSERT(flags & OF_FLAG, "negq INT64_MIN: OF should be set");
}

static void test_neg_sizes(void) {
    uint8_t r8;
    uint16_t r16;
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "movb $1, %%al\n\t"
        "negb %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0xFF, "negb 1 expected 0xFF");

    __asm__ volatile (
        "movw $1000, %%ax\n\t"
        "negw %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(r16)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == (uint16_t)-1000, "negw 1000");

    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "negl %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 0x80000000, "negl INT32_MIN: result is INT32_MIN");
    TEST_ASSERT(flags & OF_FLAG, "negl INT32_MIN: OF should be set");
}

int main(void) {
    TEST_START("NEG instruction");
    test_neg_basic();
    test_neg_zero();
    test_neg_min();
    test_neg_sizes();
    TEST_END();
}
