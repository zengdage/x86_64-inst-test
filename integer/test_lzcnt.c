/*
 * test_lzcnt.c - Test x86-64 LZCNT instruction (part of BMI1/ABM)
 *
 * LZCNT counts leading zero bits (from MSB).
 * If source is 0, result = operand size and CF is set.
 * Affects flags: CF (set if source is 0), ZF (set if result is 0)
 *
 * Compile: gcc -o test_lzcnt integer/test_lzcnt.c -O0 -mlzcnt
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_lzcnt_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* LZCNT of 1 => 63 leading zeros */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "lzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 63, "lzcntq 1: expected 63, got %lu", result);
    TEST_ASSERT(!(flags & CF_FLAG), "lzcntq 1: CF should be clear");

    /* LZCNT of 0x8000000000000000 => 0 */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "lzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "lzcntq 0x8000000000000000: expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "lzcntq result=0: ZF should be set");

    /* LZCNT of 0 => 64, CF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "lzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 64, "lzcntq 0: expected 64, got %lu", result);
    TEST_ASSERT(flags & CF_FLAG, "lzcntq 0: CF should be set");

    /* LZCNT of all-ones => 0 */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "lzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "lzcntq all-ones: expected 0");
}

static void test_lzcnt_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0x00008000, %%eax\n\t"
        "lzcntl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 16, "lzcntl 0x8000: expected 16, got %u", result);

    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "lzcntl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 0, "lzcntl 0x80000000: expected 0");
}

static void test_lzcnt_16bit(void) {
    uint16_t result;

    __asm__ volatile (
        "movw $0x0001, %%ax\n\t"
        "lzcntw %%ax, %%cx\n\t"
        "movw %%cx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 15, "lzcntw 1: expected 15, got %u", result);
}

int main(void) {
    TEST_START("LZCNT instruction");
    test_lzcnt_basic();
    test_lzcnt_32bit();
    test_lzcnt_16bit();
    TEST_END();
}
