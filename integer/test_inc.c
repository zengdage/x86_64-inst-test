/*
 * test_inc.c - Test x86-64 INC instruction
 *
 * INC increments operand by 1: DEST = DEST + 1
 * Affects flags: PF, AF, ZF, SF, OF (does NOT affect CF)
 *
 * Compile: gcc -o test_inc integer/test_inc.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_inc_basic(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "incq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "incq 0 expected 1");
    TEST_ASSERT(!(flags & ZF_FLAG), "incq 0->1: ZF should be clear");

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "incq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "incq -1 expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "incq -1->0: ZF should be set");
}

static void test_inc_overflow(void) {
    uint64_t result;
    uint64_t flags;

    /* Signed overflow: INT64_MAX + 1 */
    __asm__ volatile (
        "movabsq $0x7FFFFFFFFFFFFFFF, %%rax\n\t"
        "incq %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x8000000000000000UL, "incq INT64_MAX+1");
    TEST_ASSERT(flags & OF_FLAG, "incq INT64_MAX: OF should be set");
    TEST_ASSERT(flags & SF_FLAG, "incq INT64_MAX: SF should be set");
}

static void test_inc_cf_preserved(void) {
    uint64_t flags;

    /* INC should NOT affect CF */
    __asm__ volatile (
        "stc\n\t"
        "movq $0, %%rax\n\t"
        "incq %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "incq: CF should be preserved (was set)");

    __asm__ volatile (
        "clc\n\t"
        "movq $0, %%rax\n\t"
        "incq %%rax\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "incq: CF should be preserved (was clear)");
}

static void test_inc_sizes(void) {
    uint8_t r8;
    uint16_t r16;
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "movb $0xFF, %%al\n\t"
        "incb %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0, "incb 0xFF expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "incb 0xFF->0: ZF should be set");

    __asm__ volatile (
        "movw $0x7FFF, %%ax\n\t"
        "incw %%ax\n\t"
        "movw %%ax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r16), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 0x8000, "incw 0x7FFF expected 0x8000");
    TEST_ASSERT(flags & OF_FLAG, "incw 0x7FFF: OF should be set");

    __asm__ volatile (
        "movl $99, %%eax\n\t"
        "incl %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 100, "incl 99 expected 100");
}

static void test_inc_mem(void) {
    uint64_t val = 41;
    __asm__ volatile (
        "incq %0"
        : "+m"(val)
        :
        : "cc"
    );
    TEST_ASSERT(val == 42, "incq mem: 41+1 expected 42");
}

int main(void) {
    TEST_START("INC instruction");
    test_inc_basic();
    test_inc_overflow();
    test_inc_cf_preserved();
    test_inc_sizes();
    test_inc_mem();
    TEST_END();
}
