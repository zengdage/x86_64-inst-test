/*
 * test_test.c - Test x86-64 TEST instruction
 *
 * TEST performs bitwise AND without storing result: computes DEST & SRC and sets flags.
 * Affects flags: SF, ZF, PF (CF and OF cleared)
 *
 * Compile: gcc -o test_test integer/test_test.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_test_zero(void) {
    uint64_t flags;

    /* Disjoint bits => ZF set */
    __asm__ volatile (
        "movq $0xF0, %%rax\n\t"
        "testq $0x0F, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "testq 0xF0 & 0x0F: ZF should be set");
    TEST_ASSERT(!(flags & CF_FLAG), "testq: CF should be cleared");
    TEST_ASSERT(!(flags & OF_FLAG), "testq: OF should be cleared");
}

static void test_test_nonzero(void) {
    uint64_t flags;

    __asm__ volatile (
        "movq $0xFF, %%rax\n\t"
        "testq $0x01, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(!(flags & ZF_FLAG), "testq 0xFF & 0x01: ZF should be clear");
}

static void test_test_sf(void) {
    uint64_t flags;

    /* Test sign bit */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "testq %%rax, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & SF_FLAG, "testq negative: SF should be set");
    TEST_ASSERT(!(flags & ZF_FLAG), "testq negative: ZF should be clear");
}

static void test_test_pf(void) {
    uint64_t flags;

    /* 0xFF & 0xFF = 0xFF => 8 set bits in low byte => PF set (even count) */
    __asm__ volatile (
        "movq $0xFF, %%rax\n\t"
        "testq $0xFF, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & PF_FLAG, "testq 0xFF: PF should be set (8 bits = even)");

    /* 0x01 & 0x01 = 0x01 => 1 set bit => PF clear (odd count) */
    __asm__ volatile (
        "movq $0x01, %%rax\n\t"
        "testq $0x01, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(!(flags & PF_FLAG), "testq 0x01: PF should be clear (1 bit = odd)");
}

static void test_test_sizes(void) {
    uint64_t flags;

    /* 8-bit test */
    __asm__ volatile (
        "movb $0xAA, %%al\n\t"
        "testb $0x55, %%al\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "testb 0xAA & 0x55: ZF should be set");

    /* 32-bit test */
    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "testl $0x80000000, %%eax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & SF_FLAG, "testl 0x80000000: SF should be set");
}

static void test_test_reg_reg(void) {
    uint64_t flags;

    /* Common idiom: test rax, rax to check if zero */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "testq %%rax, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "testq rax,rax (rax=0): ZF should be set");
}

int main(void) {
    TEST_START("TEST instruction");
    test_test_zero();
    test_test_nonzero();
    test_test_sf();
    test_test_pf();
    test_test_sizes();
    test_test_reg_reg();
    TEST_END();
}
