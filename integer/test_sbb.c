/*
 * test_sbb.c - Test x86-64 SBB instruction
 *
 * SBB performs subtraction with borrow: DEST = DEST - SRC - CF
 * Affects flags: CF, PF, AF, ZF, SF, OF
 *
 * Compile: gcc -o test_sbb integer/test_sbb.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_sbb_no_borrow(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "clc\n\t"
        "movq $100, %%rax\n\t"
        "movq $30, %%rbx\n\t"
        "sbbq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 70, "sbbq CF=0: 100-30-0 expected 70, got %lu", result);
    TEST_ASSERT(!(flags & CF_FLAG), "sbbq 100-30: CF should be clear");
}

static void test_sbb_with_borrow(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "stc\n\t"
        "movq $100, %%rax\n\t"
        "movq $30, %%rbx\n\t"
        "sbbq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 69, "sbbq CF=1: 100-30-1 expected 69, got %lu", result);
}

static void test_sbb_underflow(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "stc\n\t"
        "sbbq $0, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0xFFFFFFFFFFFFFFFFUL, "sbbq 0-0-1 expected UINT64_MAX");
    TEST_ASSERT(flags & CF_FLAG, "sbbq 0-0-1: CF should be set");
    TEST_ASSERT(flags & SF_FLAG, "sbbq 0-0-1: SF should be set");
}

static void test_sbb_chain(void) {
    /* 128-bit subtraction using SBB chain */
    uint64_t lo, hi;

    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"      /* lo = 0 */
        "movq $1, %%rdx\n\t"          /* hi = 1, number = 0x1_0000000000000000 */
        "subq $1, %%rax\n\t"          /* subtract 1 from lo */
        "sbbq $0, %%rdx\n\t"          /* propagate borrow to hi */
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        :
        : "rax", "rdx", "cc"
    );
    TEST_ASSERT(lo == 0xFFFFFFFFFFFFFFFFUL && hi == 0,
                "128-bit sub: lo=0x%lx hi=%lu", lo, hi);
}

static void test_sbb_sizes(void) {
    uint8_t r8;
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "stc\n\t"
        "movb $10, %%al\n\t"
        "sbbb $5, %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 4, "sbbb 10-5-1 expected 4, got %u", r8);

    __asm__ volatile (
        "stc\n\t"
        "movl $1000, %%eax\n\t"
        "sbbl $500, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 499, "sbbl 1000-500-1 expected 499, got %u", r32);
}

static void test_sbb_complete_flags_and_memory(void) {
    uint8_t value;
    uint64_t flags;
    __asm__ volatile("stc\n\tmovb $0x80,%%al\n\tsbbb $0,%%al\n\tmovb %%al,%0\n\tpushfq\n\tpopq %1"
        : "=r"(value), "=r"(flags) : : "rax", "cc");
    TEST_ASSERT(value == 0x7f, "sbbb signed-overflow result");
    TEST_ASSERT(!(flags & CF_FLAG) && (flags & OF_FLAG) && (flags & AF_FLAG) &&
                !(flags & ZF_FLAG) && !(flags & SF_FLAG) && !(flags & PF_FLAG),
                "sbbb 0x80-CF full flags=0x%llx", (unsigned long long)flags);

    __asm__ volatile("movb $0,%%al\n\tstc\n\tsbbb $0,%%al\n\tmovb %%al,%0\n\tpushfq\n\tpopq %1"
        : "=r"(value), "=r"(flags) : : "rax", "cc");
    TEST_ASSERT(value == 0xff, "sbbb borrow result");
    TEST_ASSERT((flags & CF_FLAG) && !(flags & OF_FLAG) && (flags & AF_FLAG) &&
                !(flags & ZF_FLAG) && (flags & SF_FLAG) && (flags & PF_FLAG),
                "sbbb 0-CF full flags=0x%llx", (unsigned long long)flags);

    uint32_t mem = 0;
    __asm__ volatile("stc\n\tlock sbbl $0,%0\n\tpushfq\n\tpopq %1"
        : "+m"(mem), "=r"(flags) : : "cc", "memory");
    TEST_ASSERT(mem == UINT32_MAX && (flags & CF_FLAG) && (flags & SF_FLAG), "lock sbbl memory destination and flags");
}

int main(void) {
    TEST_START("SBB instruction");
    test_sbb_no_borrow();
    test_sbb_with_borrow();
    test_sbb_underflow();
    test_sbb_chain();
    test_sbb_sizes();
    test_sbb_complete_flags_and_memory();
    TEST_END();
}
