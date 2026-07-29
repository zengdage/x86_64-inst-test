/*
 * test_bsf_bsr.c - Test x86-64 BSF/BSR instructions
 *
 * BSF: Bit Scan Forward - finds the index of the least significant set bit
 * BSR: Bit Scan Reverse - finds the index of the most significant set bit
 * Affects flags: ZF (set if source is 0)
 *
 * Compile: gcc -o test_bsf_bsr integer/test_bsf_bsr.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_bsf(void) {
    uint64_t result;
    uint64_t flags;

    /* BSF of 1 => bit 0 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "bsfq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "bsfq 1: expected bit 0, got %lu", result);
    TEST_ASSERT(!(flags & ZF_FLAG), "bsfq 1: ZF should be clear");

    /* BSF of 0x80 => bit 7 */
    __asm__ volatile (
        "movq $0x80, %%rax\n\t"
        "bsfq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 7, "bsfq 0x80: expected bit 7, got %lu", result);

    /* BSF of 0x8000000000000000 => bit 63 */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "bsfq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 63, "bsfq 0x8000000000000000: expected bit 63");

    /* BSF of 0x100 => bit 8 */
    __asm__ volatile (
        "movq $0x100, %%rax\n\t"
        "bsfq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 8, "bsfq 0x100: expected bit 8");

    /* BSF of 0 => ZF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "bsfq %%rax, %%rbx\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "bsfq 0: ZF should be set");
}

static void test_bsr(void) {
    uint64_t result;
    uint64_t flags;

    /* BSR of 1 => bit 0 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "bsrq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "bsrq 1: expected bit 0, got %lu", result);
    TEST_ASSERT(!(flags & ZF_FLAG), "bsrq 1: ZF should be clear");

    /* BSR of 0xFF => bit 7 */
    __asm__ volatile (
        "movq $0xFF, %%rax\n\t"
        "bsrq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 7, "bsrq 0xFF: expected bit 7");

    /* BSR of UINT64_MAX => bit 63 */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "bsrq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 63, "bsrq UINT64_MAX: expected bit 63");

    /* BSR of 0 => ZF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "bsrq %%rax, %%rbx\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "bsrq 0: ZF should be set");
}

static void test_bsf_bsr_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "bsfl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 31, "bsfl 0x80000000: expected bit 31");

    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "bsrl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 31, "bsrl 0x80000000: expected bit 31");
}

int main(void) {
    TEST_START("BSF/BSR instructions");
    test_bsf();
    test_bsr();
    test_bsf_bsr_32bit();
    TEST_END();
}
