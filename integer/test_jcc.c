/*
 * test_jcc.c - Test x86-64 Jcc (conditional jump) instructions
 *
 * Tests various conditional jumps by setting up conditions and
 * verifying that the correct branch is taken.
 *
 * Compile: gcc -o test_jcc integer/test_jcc.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_je_jne(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $5, %%rax\n\t"
        "cmpq $5, %%rax\n\t"
        "jne 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "je (equal): branch taken, got %lu", result);

    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $3, %%rax\n\t"
        "cmpq $5, %%rax\n\t"
        "je 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "jne (not equal): branch not taken, got %lu", result);
}

static void test_jg_jl(void) {
    uint64_t result;

    /* JG: jump if greater (signed) */
    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $10, %%rax\n\t"
        "cmpq $5, %%rax\n\t"
        "jle 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "jg (10 > 5): branch taken");

    /* JL: jump if less (signed) */
    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $-1, %%rax\n\t"
        "cmpq $0, %%rax\n\t"
        "jge 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "jl (-1 < 0): branch taken");
}

static void test_ja_jb(void) {
    uint64_t result;

    /* JA: jump if above (unsigned) */
    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $0xFFFFFFFF, %%rax\n\t"
        "cmpq $1, %%rax\n\t"
        "jbe 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "ja (unsigned above): branch taken");

    /* JB: jump if below (unsigned) */
    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $1, %%rax\n\t"
        "cmpq $0xFF, %%rax\n\t"
        "jae 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "jb (unsigned below): branch taken");
}

static void test_js_jns(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $-1, %%rax\n\t"
        "testq %%rax, %%rax\n\t"
        "jns 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "js (negative): branch taken");

    __asm__ volatile (
        "movq $0, %0\n\t"
        "movq $1, %%rax\n\t"
        "testq %%rax, %%rax\n\t"
        "js 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "jns (positive): branch taken");
}

static void test_jo_jno(void) {
    uint64_t result;

    /* JO: overflow */
    __asm__ volatile (
        "movq $0, %0\n\t"
        "movabsq $0x7FFFFFFFFFFFFFFF, %%rax\n\t"
        "addq $1, %%rax\n\t"
        "jno 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "jo (overflow): branch taken");
}

static void test_jz_after_test(void) {
    uint64_t result;

    /* Common pattern: test + jz */
    __asm__ volatile (
        "movq $0, %0\n\t"
        "xorq %%rax, %%rax\n\t"
        "testq %%rax, %%rax\n\t"
        "jnz 1f\n\t"
        "movq $1, %0\n\t"
        "1:"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "jz (rax=0): branch taken");
}

int main(void) {
    TEST_START("Jcc instructions");
    test_je_jne();
    test_jg_jl();
    test_ja_jb();
    test_js_jns();
    test_jo_jno();
    test_jz_after_test();
    TEST_END();
}
