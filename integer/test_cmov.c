/*
 * test_cmov.c - Test x86-64 CMOVcc instructions
 *
 * CMOVcc conditionally moves SRC to DEST based on flag conditions.
 * Does not affect any flags.
 *
 * Compile: gcc -o test_cmov integer/test_cmov.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_cmove_cmovne(void) {
    uint64_t result;

    /* CMOVE: move if ZF=1 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $1, %%rax\n\t"     /* sets ZF */
        "cmoveq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmoveq (ZF=1): should move, got %lu", result);

    /* CMOVE: don't move if ZF=0 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $2, %%rax\n\t"     /* clears ZF */
        "cmoveq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 1, "cmoveq (ZF=0): should not move, got %lu", result);

    /* CMOVNE: move if ZF=0 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $2, %%rax\n\t"
        "cmovneq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovneq (ZF=0): should move");
}

static void test_cmovg_cmovl(void) {
    uint64_t result;

    /* CMOVG: move if greater (signed): ZF=0 and SF=OF */
    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $-1, %%rax\n\t"    /* 0 > -1 signed */
        "cmovgq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovgq (0 > -1): should move");

    /* CMOVL: move if less (signed): SF!=OF */
    __asm__ volatile (
        "movq $-5, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $5, %%rax\n\t"     /* -5 < 5 */
        "cmovlq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovlq (-5 < 5): should move");

    /* CMOVL: don't move if not less */
    __asm__ volatile (
        "movq $10, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $5, %%rax\n\t"     /* 10 > 5 */
        "cmovlq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 10, "cmovlq (10 > 5): should not move");
}

static void test_cmova_cmovb(void) {
    uint64_t result;

    /* CMOVA: move if above (unsigned): CF=0 and ZF=0 */
    __asm__ volatile (
        "movq $10, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $5, %%rax\n\t"
        "cmovaq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovaq (10 > 5 unsigned): should move");

    /* CMOVB: move if below (unsigned): CF=1 */
    __asm__ volatile (
        "movq $3, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $10, %%rax\n\t"
        "cmovbq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovbq (3 < 10 unsigned): should move");
}

static void test_cmovge_cmovle(void) {
    uint64_t result;

    /* CMOVGE: move if greater or equal (signed): SF=OF */
    __asm__ volatile (
        "movq $5, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $5, %%rax\n\t"     /* equal */
        "cmovgeq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovgeq (equal): should move");

    /* CMOVLE: move if less or equal: ZF=1 or SF!=OF */
    __asm__ volatile (
        "movq $5, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $5, %%rax\n\t"
        "cmovleq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovleq (equal): should move");
}

static void test_cmovs_cmovns(void) {
    uint64_t result;

    /* CMOVS: move if sign (SF=1) */
    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $1, %%rax\n\t"     /* 0-1 => negative result, SF=1 */
        "cmovsq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovsq (SF=1): should move");

    /* CMOVNS: move if not sign (SF=0) */
    __asm__ volatile (
        "movq $5, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "cmpq $3, %%rax\n\t"     /* 5-3=2 => positive, SF=0 */
        "cmovnsq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 99, "cmovnsq (SF=0): should move");
}

static void test_cmov_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $1, %%eax\n\t"
        "movl $42, %%ecx\n\t"
        "cmpl $1, %%eax\n\t"
        "cmovel %%ecx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 42, "cmovel 32-bit: should move");
}

int main(void) {
    TEST_START("CMOVcc instructions");
    test_cmove_cmovne();
    test_cmovg_cmovl();
    test_cmova_cmovb();
    test_cmovge_cmovle();
    test_cmovs_cmovns();
    test_cmov_32bit();
    TEST_END();
}
