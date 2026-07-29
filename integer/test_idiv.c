/*
 * test_idiv.c - Test x86-64 IDIV instruction
 *
 * IDIV performs signed division.
 *   64-bit: RAX = RDX:RAX / r/m64, RDX = RDX:RAX % r/m64
 * The sign of the remainder equals the sign of the dividend.
 * Flags: undefined after IDIV
 *
 * Compile: gcc -o test_idiv integer/test_idiv.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_idiv_positive(void) {
    int64_t quot, rem;

    __asm__ volatile (
        "movq $100, %%rax\n\t"
        "cqo\n\t"
        "movq $7, %%rcx\n\t"
        "idivq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 14, "idivq 100/7 quotient expected 14, got %ld", quot);
    TEST_ASSERT(rem == 2, "idivq 100/7 remainder expected 2, got %ld", rem);
}

static void test_idiv_negative_dividend(void) {
    int64_t quot, rem;

    __asm__ volatile (
        "movq $-100, %%rax\n\t"
        "cqo\n\t"
        "movq $7, %%rcx\n\t"
        "idivq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == -14, "idivq -100/7 quotient expected -14, got %ld", quot);
    TEST_ASSERT(rem == -2, "idivq -100/7 remainder expected -2, got %ld", rem);
}

static void test_idiv_negative_divisor(void) {
    int64_t quot, rem;

    __asm__ volatile (
        "movq $100, %%rax\n\t"
        "cqo\n\t"
        "movq $-7, %%rcx\n\t"
        "idivq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == -14, "idivq 100/(-7) quotient expected -14, got %ld", quot);
    TEST_ASSERT(rem == 2, "idivq 100/(-7) remainder expected 2, got %ld", rem);
}

static void test_idiv_both_negative(void) {
    int64_t quot, rem;

    __asm__ volatile (
        "movq $-100, %%rax\n\t"
        "cqo\n\t"
        "movq $-7, %%rcx\n\t"
        "idivq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 14, "idivq -100/(-7) quotient expected 14, got %ld", quot);
    TEST_ASSERT(rem == -2, "idivq -100/(-7) remainder expected -2, got %ld", rem);
}

static void test_idiv_32bit(void) {
    int32_t quot, rem;

    __asm__ volatile (
        "movl $-50, %%eax\n\t"
        "cdq\n\t"
        "movl $8, %%ecx\n\t"
        "idivl %%ecx\n\t"
        "movl %%eax, %0\n\t"
        "movl %%edx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == -6, "idivl -50/8 quotient expected -6, got %d", quot);
    TEST_ASSERT(rem == -2, "idivl -50/8 remainder expected -2, got %d", rem);
}

static void test_idiv_zero_dividend(void) {
    int64_t quot, rem;

    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "cqo\n\t"
        "movq $5, %%rcx\n\t"
        "idivq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == 0 && rem == 0, "idivq 0/5 = 0 r 0");
}

static void test_idiv_exact(void) {
    int64_t quot, rem;

    __asm__ volatile (
        "movq $-49, %%rax\n\t"
        "cqo\n\t"
        "movq $7, %%rcx\n\t"
        "idivq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(quot), "=r"(rem)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(quot == -7, "idivq -49/7 quotient expected -7");
    TEST_ASSERT(rem == 0, "idivq -49/7 remainder expected 0");
}

int main(void) {
    TEST_START("IDIV instruction");
    test_idiv_positive();
    test_idiv_negative_dividend();
    test_idiv_negative_divisor();
    test_idiv_both_negative();
    test_idiv_32bit();
    test_idiv_zero_dividend();
    test_idiv_exact();
    TEST_END();
}
