/*
 * test_popcnt.c - Test x86-64 POPCNT instruction
 *
 * POPCNT counts the number of set bits (population count) in the source.
 * Affects flags: ZF (set if result is 0); CF, PF, AF, SF, OF all cleared.
 * Requires POPCNT CPU support (CPUID.01H:ECX.POPCNT[bit 23] = 1)
 *
 * Compile: gcc -o test_popcnt integer/test_popcnt.c -O0 -mpopcnt
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_popcnt_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* All zeros */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "popcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "popcntq 0: expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "popcntq 0: ZF should be set");
    TEST_ASSERT(!(flags & CF_FLAG), "popcntq: CF should be cleared");
    TEST_ASSERT(!(flags & OF_FLAG), "popcntq: OF should be cleared");

    /* All ones */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "popcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 64, "popcntq all-ones: expected 64, got %lu", result);
    TEST_ASSERT(!(flags & ZF_FLAG), "popcntq 64: ZF should be clear");

    /* Single bit */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "popcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 1, "popcntq single bit: expected 1");

    /* 0xAAAAAAAAAAAAAAAA => 32 bits set */
    __asm__ volatile (
        "movabsq $0xAAAAAAAAAAAAAAAA, %%rax\n\t"
        "popcntq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 32, "popcntq 0xAA..AA: expected 32");
}

static void test_popcnt_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0xFF, %%eax\n\t"
        "popcntl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 8, "popcntl 0xFF: expected 8, got %u", result);

    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "popcntl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 13, "popcntl 0x12345678: expected 13, got %u", result);
}

static void test_popcnt_16bit(void) {
    uint16_t result;

    __asm__ volatile (
        "movw $0xFFFF, %%ax\n\t"
        "popcntw %%ax, %%cx\n\t"
        "movw %%cx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 16, "popcntw 0xFFFF: expected 16");
}

int main(void) {
    TEST_START("POPCNT instruction");
    test_popcnt_basic();
    test_popcnt_32bit();
    test_popcnt_16bit();
    TEST_END();
}
