/*
 * test_div.c - Test x86-64 DIV instruction
 *
 * DIV performs unsigned division.
 *   8-bit:  AL = AX / r/m8,       AH = AX % r/m8
 *  32-bit:  EAX = EDX:EAX / r/m32, EDX = EDX:EAX % r/m32
 *  64-bit:  RAX = RDX:RAX / r/m64, RDX = RDX:RAX % r/m64
 * Flags: undefined after DIV
 *
 * Compile: gcc -o test_div integer/test_div.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_div_8bit(void) {
    uint8_t quot, rem;

    /* 100 / 10 = 10 remainder 0 */
    __asm__ volatile (
        "movw $100, %%ax\n\t"
        "movb $10, %%cl\n\t"
        "divb %%cl\n\t"
        "movb %%al, %0\n\t"
        "movb %%ah, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(quot == 10, "divb 100/10 quotient expected 10, got %u", quot);
    TEST_ASSERT(rem == 0, "divb 100/10 remainder expected 0, got %u", rem);

    /* 255 / 7 = 36 remainder 3 */
    __asm__ volatile (
        "movw $255, %%ax\n\t"
        "movb $7, %%cl\n\t"
        "divb %%cl\n\t"
        "movb %%al, %0\n\t"
        "movb %%ah, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(quot == 36, "divb 255/7 quotient expected 36, got %u", quot);
    TEST_ASSERT(rem == 3, "divb 255/7 remainder expected 3, got %u", rem);
}

static void test_div_32bit(void) {
    uint32_t quot, rem;

    __asm__ volatile (
        "movl $1000000, %%eax\n\t"
        "xorl %%edx, %%edx\n\t"
        "movl $300, %%ecx\n\t"
        "divl %%ecx\n\t"
        "movl %%eax, %0\n\t"
        "movl %%edx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 3333, "divl 1000000/300 quotient expected 3333, got %u", quot);
    TEST_ASSERT(rem == 100, "divl 1000000/300 remainder expected 100, got %u", rem);
}

static void test_div_64bit(void) {
    uint64_t quot, rem;

    /* Simple: 100 / 7 */
    __asm__ volatile (
        "movq $100, %%rax\n\t"
        "xorq %%rdx, %%rdx\n\t"
        "movq $7, %%rcx\n\t"
        "divq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 14, "divq 100/7 quotient expected 14, got %lu", quot);
    TEST_ASSERT(rem == 2, "divq 100/7 remainder expected 2, got %lu", rem);

    /* Division of 1 by 1 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "xorq %%rdx, %%rdx\n\t"
        "movq $1, %%rcx\n\t"
        "divq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 1 && rem == 0, "divq 1/1 = 1 r 0");

    /* Large dividend: UINT64_MAX / 2 */
    __asm__ volatile (
        "movq $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
        "xorq %%rdx, %%rdx\n\t"
        "movq $2, %%rcx\n\t"
        "divq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 0x7FFFFFFFFFFFFFFFUL, "divq UINT64_MAX/2 quotient");
    TEST_ASSERT(rem == 1, "divq UINT64_MAX/2 remainder expected 1");

    /* 0 / anything = 0 */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "xorq %%rdx, %%rdx\n\t"
        "movq $42, %%rcx\n\t"
        "divq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 0 && rem == 0, "divq 0/42 = 0 r 0");
}

static void test_div_mem(void) {
    uint64_t quot, rem;
    uint64_t divisor = 10;

    __asm__ volatile (
        "movq $123, %%rax\n\t"
        "xorq %%rdx, %%rdx\n\t"
        "divq %2\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        : "m"(divisor)
        : "rax", "rdx", "cc"
    );
    TEST_ASSERT(quot == 12, "divq mem: 123/10 quotient expected 12");
    TEST_ASSERT(rem == 3, "divq mem: 123/10 remainder expected 3");
}

int main(void) {
    TEST_START("DIV instruction");
    test_div_8bit();
    test_div_32bit();
    test_div_64bit();
    test_div_mem();
    TEST_END();
}
