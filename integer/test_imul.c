/*
 * test_imul.c - Test x86-64 IMUL instruction
 *
 * IMUL performs signed multiplication. Three forms:
 *   One operand:   RDX:RAX = RAX * r/m64
 *   Two operand:   DEST = DEST * r/m
 *   Three operand: DEST = r/m * imm
 * Affects flags: CF, OF (set if result overflows destination size)
 *
 * Compile: gcc -o test_imul integer/test_imul.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_imul_one_operand(void) {
    int64_t lo, hi;
    uint64_t flags;

    /* Positive * Positive */
    __asm__ volatile (
        "movq $100, %%rax\n\t"
        "movq $200, %%rcx\n\t"
        "imulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=r"(lo), "=r"(hi), "=r"(flags)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == 20000 && hi == 0, "imulq 100*200 = 20000");
    TEST_ASSERT(!(flags & OF_FLAG), "imulq 100*200: OF clear");

    /* Negative * Positive */
    __asm__ volatile (
        "movq $-5, %%rax\n\t"
        "movq $10, %%rcx\n\t"
        "imulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == -50, "imulq -5*10 = -50");
    TEST_ASSERT(hi == -1, "imulq -5*10: hi should be sign extension (-1)");

    /* Negative * Negative */
    __asm__ volatile (
        "movq $-7, %%rax\n\t"
        "movq $-8, %%rcx\n\t"
        "imulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == 56, "imulq -7*-8 = 56");
    TEST_ASSERT(hi == 0, "imulq -7*-8: hi should be 0");
}

static void test_imul_two_operand(void) {
    int64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $-15, %%rax\n\t"
        "movq $3, %%rbx\n\t"
        "imulq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == -45, "imulq 2-op: -15*3 = -45, got %ld", result);
}

static void test_imul_three_operand(void) {
    int64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $25, %%rbx\n\t"
        "imulq $4, %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 100, "imulq 3-op: 25*4 = 100, got %ld", result);
    TEST_ASSERT(!(flags & OF_FLAG), "imulq 3-op 25*4: OF clear");

    /* Overflow test */
    __asm__ volatile (
        "movabsq $0x7FFFFFFFFFFFFFFF, %%rbx\n\t"
        "imulq $2, %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(flags & OF_FLAG, "imulq 3-op INT64_MAX*2: OF should be set");
}

static void test_imul_32bit(void) {
    int32_t result;
    uint64_t flags;

    __asm__ volatile (
        "movl $-10, %%eax\n\t"
        "movl $-10, %%ecx\n\t"
        "imull %%ecx, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 100, "imull -10*-10 = 100, got %d", result);

    /* 32-bit overflow */
    __asm__ volatile (
        "movl $0x7FFFFFFF, %%eax\n\t"
        "imull $2, %%eax, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & OF_FLAG, "imull INT32_MAX*2: OF should be set");
}

static void test_imul_zero(void) {
    int64_t lo, hi;

    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "movq $0x123456, %%rcx\n\t"
        "imulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == 0 && hi == 0, "imulq 0*anything = 0");
}

static void test_imul_boundaries(void) {
    int64_t lo, hi, result;
    uint64_t flags;

    /* INT64_MIN * 1 is the lowest representable signed result. */
    __asm__ volatile (
        "movabsq $0x8000000000000000, %%rax\n\t"
        "movq $1, %%rcx\n\t"
        "imulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=r"(lo), "=r"(hi), "=r"(flags)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == INT64_MIN && hi == -1, "imulq INT64_MIN*1 full result");
    TEST_ASSERT(!(flags & (CF_FLAG | OF_FLAG)), "imulq INT64_MIN*1: no overflow");

    /* Negating INT64_MIN via multiplication cannot fit in 64 bits. */
    __asm__ volatile (
        "movabsq $0x8000000000000000, %%rax\n\t"
        "imulq $-1, %%rax, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == INT64_MIN, "imulq INT64_MIN*-1: truncated result wraps");
    TEST_ASSERT((flags & (CF_FLAG | OF_FLAG)) == (CF_FLAG | OF_FLAG),
                "imulq INT64_MIN*-1: CF and OF set");

    /* The one-operand form retains the exact positive 2^63 result. */
    __asm__ volatile (
        "movabsq $0x8000000000000000, %%rax\n\t"
        "movq $-1, %%rcx\n\t"
        "imulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT((uint64_t)lo == UINT64_C(0x8000000000000000) && hi == 0,
                "imulq one-operand INT64_MIN*-1: exact 128-bit result");
}

int main(void) {
    TEST_START("IMUL instruction");
    test_imul_one_operand();
    test_imul_two_operand();
    test_imul_three_operand();
    test_imul_32bit();
    test_imul_zero();
    test_imul_boundaries();
    TEST_END();
}
