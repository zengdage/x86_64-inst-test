/*
 * test_tzcnt.c - Test x86-64 TZCNT instruction
 *
 * TZCNT counts trailing zero bits (from LSB).
 * If source is 0, result = operand size and CF is set.
 * Affects flags: CF (set if source is 0), ZF (set if result is 0)
 * Requires BMI1 CPU support.
 *
 * Compile: gcc -o test_tzcnt integer/test_tzcnt.c -O0 -mbmi
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_tzcnt_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* TZCNT of 1 => 0 trailing zeros */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "tzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "tzcntq 1: expected 0, got %lu", result);
    TEST_ASSERT(flags & ZF_FLAG, "tzcntq 1: ZF should be set (result=0)");
    TEST_ASSERT(!(flags & CF_FLAG), "tzcntq 1: CF should be clear");

    /* TZCNT of 0x80 => 7 trailing zeros */
    __asm__ volatile (
        "movq $0x80, %%rax\n\t"
        "tzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 7, "tzcntq 0x80: expected 7, got %lu", result);

    /* TZCNT of 0 => 64, CF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "tzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 64, "tzcntq 0: expected 64, got %lu", result);
    TEST_ASSERT(flags & CF_FLAG, "tzcntq 0: CF should be set");

    /* TZCNT of 0x8000000000000000 => 63 */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "tzcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 63, "tzcntq 0x8000000000000000: expected 63");
}

static void test_tzcnt_32bit(void) {
    uint32_t result;
    uint64_t flags;

    __asm__ volatile (
        "movl $0x100, %%eax\n\t"
        "tzcntl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 8, "tzcntl 0x100: expected 8, got %u", result);

    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "tzcntl %%eax, %%ecx\n\t"
        "movl %%ecx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 32, "tzcntl 0: expected 32");
    TEST_ASSERT(flags & CF_FLAG, "tzcntl 0: CF should be set");
}

static void test_tzcnt_16bit(void) {
    uint16_t result;

    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "tzcntw %%ax, %%cx\n\t"
        "movw %%cx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 15, "tzcntw 0x8000: expected 15, got %u", result);
}

int main(void) {
    TEST_START("TZCNT instruction");
    test_tzcnt_basic();
    test_tzcnt_32bit();
    test_tzcnt_16bit();
    TEST_END();
}
