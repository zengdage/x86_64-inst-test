/*
 * test_bzhi.c - Test x86-64 BZHI instruction (BMI2)
 *
 * BZHI: Zero high bits starting from specified bit position.
 * BZHI dest, src, index: dest = src & ((1 << index) - 1) for index < 64
 * Affects flags: ZF, SF, CF (CF set if index > operand size); OF cleared
 *
 * Compile: gcc -o test_bzhi integer/test_bzhi.c -O0 -mbmi2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_bzhi_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* Zero bits from bit 8 upward */
    __asm__ volatile (
        "movq $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
        "movq $8, %%rbx\n\t"
        "bzhiq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xFF, "bzhiq index=8: expected 0xFF, got 0x%lx", result);
    TEST_ASSERT(!(flags & CF_FLAG), "bzhiq index=8: CF should be clear");
}

static void test_bzhi_zero_index(void) {
    uint64_t result;
    uint64_t flags;

    __asm__ volatile (
        "movq $0xFFFFFFFF, %%rax\n\t"
        "xorq %%rbx, %%rbx\n\t"
        "bzhiq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0, "bzhiq index=0: expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "bzhiq index=0: ZF should be set");
}

static void test_bzhi_full(void) {
    uint64_t result;
    uint64_t flags;

    /* Index >= 64 => no masking, CF set */
    __asm__ volatile (
        "movq $0xDEADBEEF, %%rax\n\t"
        "movq $64, %%rbx\n\t"
        "bzhiq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xDEADBEEF, "bzhiq index=64: no masking");
    TEST_ASSERT(flags & CF_FLAG, "bzhiq index>=64: CF should be set");
}

static void test_bzhi_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0xFFFFFFFF, %%eax\n\t"
        "movl $16, %%ebx\n\t"
        "bzhil %%ebx, %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xFFFF, "bzhil index=16: expected 0xFFFF, got 0x%x", result);
}

static void test_bzhi_various(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "movq $32, %%rbx\n\t"
        "bzhiq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x9ABCDEF0UL, "bzhiq index=32: low 32 bits");

    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "movq $4, %%rbx\n\t"
        "bzhiq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x0, "bzhiq index=4 of 0x...F0: expected 0x0, got 0x%lx", result);
}

int main(void) {
    TEST_START("BZHI instruction (BMI2)");
    test_bzhi_basic();
    test_bzhi_zero_index();
    test_bzhi_full();
    test_bzhi_32bit();
    test_bzhi_various();
    TEST_END();
}
