/*
 * test_setcc.c - Test x86-64 SETcc instructions
 *
 * SETcc sets a byte to 1 if condition is true, 0 otherwise.
 * Does not affect any flags.
 *
 * Compile: gcc -o test_setcc integer/test_setcc.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_sete_setne(void) {
    uint8_t result;

    __asm__ volatile (
        "cmpq $5, %1\n\t"
        "sete %0"
        : "=r"(result)
        : "r"((uint64_t)5)
        : "cc"
    );
    TEST_ASSERT(result == 1, "sete (equal): expected 1, got %u", result);

    __asm__ volatile (
        "cmpq $5, %1\n\t"
        "setne %0"
        : "=r"(result)
        : "r"((uint64_t)5)
        : "cc"
    );
    TEST_ASSERT(result == 0, "setne (equal): expected 0, got %u", result);

    __asm__ volatile (
        "cmpq $3, %1\n\t"
        "setne %0"
        : "=r"(result)
        : "r"((uint64_t)5)
        : "cc"
    );
    TEST_ASSERT(result == 1, "setne (not equal): expected 1");
}

static void test_setg_setl(void) {
    uint8_t result;

    /* SETG: set if greater (signed) */
    __asm__ volatile (
        "movq $10, %%rax\n\t"
        "cmpq $5, %%rax\n\t"
        "setg %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "setg (10 > 5): expected 1");

    __asm__ volatile (
        "movq $3, %%rax\n\t"
        "cmpq $5, %%rax\n\t"
        "setg %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "setg (3 > 5): expected 0");

    /* SETL: set if less (signed) */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "cmpq $0, %%rax\n\t"
        "setl %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "setl (-1 < 0): expected 1");
}

static void test_seta_setb(void) {
    uint8_t result;

    /* SETA: set if above (unsigned) */
    __asm__ volatile (
        "movq $0xFFFFFFFF, %%rax\n\t"
        "cmpq $1, %%rax\n\t"
        "seta %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "seta (0xFFFFFFFF > 1 unsigned): expected 1");

    /* SETB: set if below (unsigned) */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "movl $0xFFFFFFFF, %%ecx\n\t"
        "cmpq %%rcx, %%rax\n\t"
        "setb %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 1, "setb (1 < 0xFFFFFFFF unsigned): expected 1");
}

static void test_sets_setns(void) {
    uint8_t result;

    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "subq $1, %%rax\n\t"
        "sets %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "sets (negative result): expected 1");

    __asm__ volatile (
        "movq $5, %%rax\n\t"
        "subq $1, %%rax\n\t"
        "setns %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "setns (positive result): expected 1");
}

static void test_seto_setno(void) {
    uint8_t result;

    /* Overflow: INT64_MAX + 1 */
    __asm__ volatile (
        "movabsq $0x7FFFFFFFFFFFFFFF, %%rax\n\t"
        "addq $1, %%rax\n\t"
        "seto %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "seto (overflow): expected 1");

    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "addq $1, %%rax\n\t"
        "setno %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "setno (no overflow): expected 1");
}

static void test_setc_setnc(void) {
    uint8_t result;

    __asm__ volatile (
        "stc\n\t"
        "setc %0"
        : "=r"(result)
        :
        : "cc"
    );
    TEST_ASSERT(result == 1, "setc (CF=1): expected 1");

    __asm__ volatile (
        "clc\n\t"
        "setnc %0"
        : "=r"(result)
        :
        : "cc"
    );
    TEST_ASSERT(result == 1, "setnc (CF=0): expected 1");
}

static void test_setcc_mem(void) {
    uint8_t result = 0xFF;

    __asm__ volatile (
        "cmpq $0, %1\n\t"
        "sete %0"
        : "=m"(result)
        : "r"((uint64_t)0)
        : "cc"
    );
    TEST_ASSERT(result == 1, "sete to memory: expected 1, got %u", result);
}

int main(void) {
    TEST_START("SETcc instructions");
    test_sete_setne();
    test_setg_setl();
    test_seta_setb();
    test_sets_setns();
    test_seto_setno();
    test_setc_setnc();
    test_setcc_mem();
    TEST_END();
}
