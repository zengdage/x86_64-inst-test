/*
 * test_blsi_blsmsk_blsr.c - Test x86-64 BLSI/BLSMSK/BLSR instructions (BMI1)
 *
 * BLSI:   Extract lowest set bit: dest = src & (-src)
 * BLSMSK: Get mask up to lowest set bit: dest = src ^ (src - 1)
 * BLSR:   Reset lowest set bit: dest = src & (src - 1)
 * Affects flags: ZF, SF, CF (OF cleared for BLSI/BLSR; OF, SF undefined for BLSMSK)
 *
 * Compile: gcc -o test_blsi_blsmsk_blsr integer/test_blsi_blsmsk_blsr.c -O0 -mbmi
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_blsi(void) {
    uint64_t result;
    uint64_t flags;

    /* BLSI: isolate lowest set bit */
    __asm__ volatile (
        "movq $0b101100, %%rax\n\t"
        "blsiq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0b100, "blsiq 0b101100: lowest bit = 0b100, got 0x%lx", result);
    TEST_ASSERT(!(flags & ZF_FLAG), "blsiq non-zero: ZF clear");

    /* BLSI of 0 => 0, ZF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "blsiq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "blsiq 0: expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "blsiq 0: ZF should be set");

    /* BLSI of power of 2 => same value */
    __asm__ volatile (
        "movq $0x8000, %%rax\n\t"
        "blsiq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x8000, "blsiq power-of-2: unchanged");
}

static void test_blsmsk(void) {
    uint64_t result;
    uint64_t flags;

    /* BLSMSK: mask up to and including lowest set bit */
    __asm__ volatile (
        "movq $0b101000, %%rax\n\t"
        "blsmskq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    /* 0b101000 ^ (0b101000 - 1) = 0b101000 ^ 0b100111 = 0b001111 */
    TEST_ASSERT(result == 0b001111, "blsmskq 0b101000: expected 0b001111, got 0x%lx", result);

    /* BLSMSK of 1 => 1 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "blsmskq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 1, "blsmskq 1: expected 1");

    /* BLSMSK of 0x80 => 0xFF */
    __asm__ volatile (
        "movq $0x80, %%rax\n\t"
        "blsmskq %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0xFF, "blsmskq 0x80: expected 0xFF");
}

static void test_blsr(void) {
    uint64_t result;
    uint64_t flags;

    /* BLSR: reset lowest set bit */
    __asm__ volatile (
        "movq $0b101100, %%rax\n\t"
        "blsrq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0b101000, "blsrq 0b101100: expected 0b101000, got 0x%lx", result);

    /* BLSR of power of 2 => 0 */
    __asm__ volatile (
        "movq $0x100, %%rax\n\t"
        "blsrq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "blsrq power-of-2: expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "blsrq power-of-2: ZF should be set");

    /* BLSR of 0 => 0, CF set */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "blsrq %%rax, %%rbx\n\t"
        "movq %%rbx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0, "blsrq 0: expected 0");
    TEST_ASSERT(flags & CF_FLAG, "blsrq 0: CF should be set");
}

static void test_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0xFF00, %%eax\n\t"
        "blsil %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x100, "blsil 0xFF00: lowest bit = 0x100, got 0x%x", result);

    __asm__ volatile (
        "movl $0xFF00, %%eax\n\t"
        "blsrl %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xFE00, "blsrl 0xFF00: expected 0xFE00, got 0x%x", result);
}

int main(void) {
    TEST_START("BLSI/BLSMSK/BLSR instructions (BMI1)");
    test_blsi();
    test_blsmsk();
    test_blsr();
    test_32bit();
    TEST_END();
}
