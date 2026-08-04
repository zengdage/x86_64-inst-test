/*
 * test_cmp.c - Test x86-64 CMP instruction
 *
 * CMP performs subtraction without storing result: computes DEST - SRC and sets flags.
 * Affects flags: CF, PF, AF, ZF, SF, OF
 *
 * Compile: gcc -o test_cmp integer/test_cmp.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void assert_cmp_flags(uint64_t flags, int cf, int pf, int af,
                             int zf, int sf, int of, const char *name) {
    TEST_ASSERT(!!(flags & CF_FLAG) == cf, "%s CF=%d expected %d", name,
                !!(flags & CF_FLAG), cf);
    TEST_ASSERT(!!(flags & PF_FLAG) == pf, "%s PF=%d expected %d", name,
                !!(flags & PF_FLAG), pf);
    TEST_ASSERT(!!(flags & AF_FLAG) == af, "%s AF=%d expected %d", name,
                !!(flags & AF_FLAG), af);
    TEST_ASSERT(!!(flags & ZF_FLAG) == zf, "%s ZF=%d expected %d", name,
                !!(flags & ZF_FLAG), zf);
    TEST_ASSERT(!!(flags & SF_FLAG) == sf, "%s SF=%d expected %d", name,
                !!(flags & SF_FLAG), sf);
    TEST_ASSERT(!!(flags & OF_FLAG) == of, "%s OF=%d expected %d", name,
                !!(flags & OF_FLAG), of);
}

static void test_cmp_equal(void) {
    uint64_t flags;

    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "cmpq $42, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "cmpq 42,42: ZF should be set (equal)");
    TEST_ASSERT(!(flags & CF_FLAG), "cmpq 42,42: CF should be clear");
    TEST_ASSERT(!(flags & SF_FLAG), "cmpq 42,42: SF should be clear");
}

static void test_cmp_greater(void) {
    uint64_t flags;

    /* Unsigned: 10 > 5 => CF=0, ZF=0 */
    __asm__ volatile (
        "movq $10, %%rax\n\t"
        "cmpq $5, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "cmpq 10,5: CF clear (above)");
    TEST_ASSERT(!(flags & ZF_FLAG), "cmpq 10,5: ZF clear (not equal)");
}

static void test_cmp_less(void) {
    uint64_t flags;

    /* Unsigned: 5 < 10 => CF=1 (borrow) */
    __asm__ volatile (
        "movq $5, %%rax\n\t"
        "cmpq $10, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "cmpq 5,10: CF set (below)");
    TEST_ASSERT(flags & SF_FLAG, "cmpq 5,10: SF should be set");
}

static void test_cmp_signed(void) {
    uint64_t flags;

    /* Signed: -1 vs 1 */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "cmpq $1, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & SF_FLAG, "cmpq -1,1: SF should be set");
    /* For signed less: SF != OF */

    /* Signed overflow comparison */
    __asm__ volatile (
        "movabsq $0x7FFFFFFFFFFFFFFF, %%rax\n\t"
        "cmpq $-1, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    /* INT64_MAX - (-1) = INT64_MAX + 1 => overflow */
    TEST_ASSERT(flags & OF_FLAG, "cmpq INT64_MAX,-1: OF should be set");
}

static void test_cmp_sizes(void) {
    uint64_t flags;

    /* 8-bit compare */
    __asm__ volatile (
        "movb $0xFF, %%al\n\t"
        "cmpb $0xFF, %%al\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "cmpb 0xFF,0xFF: ZF should be set");

    /* 16-bit compare */
    __asm__ volatile (
        "movw $0x100, %%ax\n\t"
        "cmpw $0x200, %%ax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "cmpw 0x100,0x200: CF set (below)");

    /* 32-bit compare */
    __asm__ volatile (
        "movl $1000, %%eax\n\t"
        "movl $500, %%ecx\n\t"
        "cmpl %%ecx, %%eax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "cmpl 1000,500: CF clear (above)");
}

static void test_cmp_reg_mem(void) {
    uint64_t flags;
    uint64_t mem_val = 100;

    __asm__ volatile (
        "movq $100, %%rax\n\t"
        "cmpq %1, %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(mem_val)
        : "rax", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "cmpq reg,mem: equal");
}

static void test_cmp_boundary_flag_matrix(void) {
    uint64_t flags, value;
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "cmpb $1, %%al\n\t"
        "pushfq\n\tpopq %0\n\t"
        "movq %%rax, %1"
        : "=r"(flags), "=r"(value) : : "rax", "cc");
    TEST_ASSERT(value == 0, "cmpb immediate does not modify zero operand");
    assert_cmp_flags(flags, 1, 1, 1, 0, 1, 0, "cmpb 0-1");

    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "cmpw $1, %%ax\n\t"
        "pushfq\n\tpopq %0\n\t"
        "movzwq %%ax, %1"
        : "=r"(flags), "=r"(value) : : "rax", "cc");
    TEST_ASSERT(value == UINT16_C(0x8000),
                "cmpw immediate preserves INT16_MIN operand");
    assert_cmp_flags(flags, 0, 1, 1, 0, 0, 1,
                     "cmpw INT16_MIN-1");

    uint32_t left32, right32;
    __asm__ volatile (
        "movl $-1, %%eax\n\t"
        "movl $-1, %%ecx\n\t"
        "cmpl %%ecx, %%eax\n\t"
        "pushfq\n\tpopq %0\n\t"
        "movl %%eax, %1\n\t"
        "movl %%ecx, %2"
        : "=r"(flags), "=r"(left32), "=r"(right32)
        : : "rax", "rcx", "cc");
    TEST_ASSERT(left32 == UINT32_MAX && right32 == UINT32_MAX,
                "cmpl register form preserves both all-ones operands");
    assert_cmp_flags(flags, 0, 1, 0, 1, 0, 0,
                     "cmpl UINT32_MAX-UINT32_MAX");

    uint64_t memory = UINT64_MAX;
    __asm__ volatile (
        "movabsq $0x7fffffffffffffff, %%rax\n\t"
        "cmpq %2, %%rax\n\t"
        "pushfq\n\tpopq %0\n\t"
        "movq %%rax, %1"
        : "=r"(flags), "=r"(value)
        : "m"(memory) : "rax", "cc");
    TEST_ASSERT(value == INT64_MAX && memory == UINT64_MAX,
                "cmpq memory form preserves signed-max/all-ones operands");
    assert_cmp_flags(flags, 1, 1, 0, 0, 1, 1,
                     "cmpq INT64_MAX-(-1)");
}

int main(void) {
    TEST_START("CMP instruction");
    test_cmp_equal();
    test_cmp_greater();
    test_cmp_less();
    test_cmp_signed();
    test_cmp_sizes();
    test_cmp_reg_mem();
    test_cmp_boundary_flag_matrix();
    TEST_END();
}
