/*
 * test_mul.c - Test x86-64 MUL instruction
 *
 * MUL performs unsigned multiplication.
 *   8-bit:  AX = AL * r/m8
 *  16-bit:  DX:AX = AX * r/m16
 *  32-bit:  EDX:EAX = EAX * r/m32
 *  64-bit:  RDX:RAX = RAX * r/m64
 * Affects flags: CF, OF (set if upper half is non-zero)
 *
 * Compile: gcc -o test_mul integer/test_mul.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_mul_8bit(void) {
    uint16_t result;
    uint64_t flags;

    __asm__ volatile (
        "movb $10, %%al\n\t"
        "movb $20, %%bl\n\t"
        "mulb %%bl\n\t"
        "movw %%ax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 200, "mulb 10*20 expected 200, got %u", result);
    TEST_ASSERT(!(flags & CF_FLAG), "mulb 10*20: CF should be clear (fits in AL)");

    /* Overflow into AH */
    __asm__ volatile (
        "movb $0xFF, %%al\n\t"
        "movb $2, %%bl\n\t"
        "mulb %%bl\n\t"
        "movw %%ax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 510, "mulb 255*2 expected 510, got %u", result);
    TEST_ASSERT(flags & CF_FLAG, "mulb 255*2: CF should be set");
}

static void test_mul_32bit(void) {
    uint32_t lo, hi;
    uint64_t flags;

    __asm__ volatile (
        "movl $100, %%eax\n\t"
        "movl $200, %%ecx\n\t"
        "mull %%ecx\n\t"
        "movl %%eax, %0\n\t"
        "movl %%edx, %1\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=r"(lo), "=r"(hi), "=r"(flags)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == 20000 && hi == 0, "mull 100*200 expected 20000");
    TEST_ASSERT(!(flags & CF_FLAG), "mull 100*200: CF clear");

    /* Overflow */
    __asm__ volatile (
        "movl $0xFFFFFFFF, %%eax\n\t"
        "movl $0xFFFFFFFF, %%ecx\n\t"
        "mull %%ecx\n\t"
        "movl %%eax, %0\n\t"
        "movl %%edx, %1\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=r"(lo), "=r"(hi), "=r"(flags)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    /* 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE_00000001 */
    TEST_ASSERT(lo == 1 && hi == 0xFFFFFFFE, "mull max*max");
    TEST_ASSERT(flags & CF_FLAG, "mull max*max: CF should be set");
}

static void test_mul_64bit(void) {
    uint64_t lo, hi;
    uint64_t flags;

    __asm__ volatile (
        "movq $1000000, %%rax\n\t"
        "movq $1000000, %%rcx\n\t"
        "mulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=r"(lo), "=r"(hi), "=r"(flags)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == 1000000000000UL && hi == 0, "mulq 10^6 * 10^6");
    TEST_ASSERT(!(flags & CF_FLAG), "mulq no overflow: CF clear");

    /* Multiply by 0 */
    __asm__ volatile (
        "movq $0xDEADBEEF, %%rax\n\t"
        "xorq %%rcx, %%rcx\n\t"
        "mulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == 0 && hi == 0, "mulq by 0: expected 0");
}

static void test_mul_mem(void) {
    uint64_t lo, hi;
    uint64_t mem_val = 7;

    __asm__ volatile (
        "movq $6, %%rax\n\t"
        "mulq %2\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        : "m"(mem_val)
        : "rax", "rdx", "cc"
    );
    TEST_ASSERT(lo == 42 && hi == 0, "mulq mem: 6*7 expected 42");
}

static void test_mul_64bit_boundaries(void) {
    uint64_t lo, hi, flags;

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "movq $-1, %%rcx\n\t"
        "mulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=r"(lo), "=r"(hi), "=r"(flags)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == 1 && hi == UINT64_MAX - 1,
                "mulq UINT64_MAX squared: full 128-bit result");
    TEST_ASSERT((flags & (CF_FLAG | OF_FLAG)) == (CF_FLAG | OF_FLAG),
                "mulq UINT64_MAX squared: CF and OF set");

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "movq $1, %%rcx\n\t"
        "mulq %%rcx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=r"(lo), "=r"(hi), "=r"(flags)
        :
        : "rax", "rcx", "rdx", "cc"
    );
    TEST_ASSERT(lo == UINT64_MAX && hi == 0, "mulq UINT64_MAX*1 boundary");
    TEST_ASSERT(!(flags & (CF_FLAG | OF_FLAG)), "mulq UINT64_MAX*1: CF and OF clear");
}

int main(void) {
    TEST_START("MUL instruction");
    test_mul_8bit();
    test_mul_32bit();
    test_mul_64bit();
    test_mul_mem();
    test_mul_64bit_boundaries();
    TEST_END();
}
