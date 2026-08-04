/*
 * test_xor.c - Test x86-64 XOR instruction
 *
 * XOR performs bitwise exclusive OR: DEST = DEST ^ SRC
 * Affects flags: SF, ZF, PF (CF and OF cleared, AF undefined)
 *
 * Compile: gcc -o test_xor integer/test_xor.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_xor_reg_reg(void) {
    uint64_t result;
    uint64_t flags;

    /* XOR with self => 0 (common idiom) */
    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "xorq %%rax, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "xorq self: expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "xorq self: ZF should be set");
    TEST_ASSERT(!(flags & CF_FLAG), "xorq: CF should be cleared");
    TEST_ASSERT(!(flags & OF_FLAG), "xorq: OF should be cleared");

    /* XOR swap test */
    uint64_t a, b;
    __asm__ volatile (
        "movq $0xAAAA, %%rax\n\t"
        "movq $0x5555, %%rbx\n\t"
        "xorq %%rbx, %%rax\n\t"
        "xorq %%rax, %%rbx\n\t"
        "xorq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rbx, %1"
        : "=r"(a), "=r"(b)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(a == 0x5555 && b == 0xAAAA, "xorq swap: a=%lu b=%lu", a, b);

    /* XOR double application => identity */
    __asm__ volatile (
        "movq $0xDEADBEEFCAFEBABE, %%rax\n\t"
        "movq $0xFFFFFFFFFFFFFFFF, %%rbx\n\t"
        "xorq %%rbx, %%rax\n\t"
        "xorq %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0xDEADBEEFCAFEBABEUL, "xorq double: identity");
}

static void test_xor_reg_imm(void) {
    uint32_t r32;
    uint64_t flags;

    __asm__ volatile (
        "movl $0xFF, %%eax\n\t"
        "xorl $0xFF, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 0, "xorl 0xFF^0xFF expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "xorl result=0: ZF should be set");

    /* toggle bits */
    __asm__ volatile (
        "movl $0xAAAAAAAA, %%eax\n\t"
        "xorl $0xFFFFFFFF, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 0x55555555, "xorl toggle bits: expected 0x55555555, got 0x%x", r32);
}

static void test_xor_sizes(void) {
    uint8_t r8;
    uint16_t r16;

    __asm__ volatile (
        "movb $0xAA, %%al\n\t"
        "xorb $0x55, %%al\n\t"
        "movb %%al, %0"
        : "=r"(r8)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0xFF, "xorb 0xAA^0x55 expected 0xFF");

    __asm__ volatile (
        "movw $0x1234, %%ax\n\t"
        "xorw $0x1234, %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(r16)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 0, "xorw self: expected 0");
}

static void test_xor_width_memory_immediate_and_flags(void) {
    uint8_t m8 = 0;
    uint16_t m16 = 0;
    uint32_t m32 = 0;
    uint64_t m64 = 0;
    uint64_t result, flags;

    __asm__ volatile ("xorb $0x80, %0" : "+m"(m8) : : "cc");
    __asm__ volatile ("xorw $0x8001, %0" : "+m"(m16) : : "cc");
    __asm__ volatile ("xorl $0x80000001, %0" : "+m"(m32) : : "cc");
    __asm__ volatile (
        "xorq $-2147483648, %0\n\t"
        "pushfq\n\tpopq %1"
        : "+m"(m64), "=r"(flags) : : "cc"
    );
    TEST_ASSERT(m8 == UINT8_C(0x80), "xorb memory boundary: %#x", m8);
    TEST_ASSERT(m16 == UINT16_C(0x8001), "xorw memory boundary: %#x", m16);
    TEST_ASSERT(m32 == UINT32_C(0x80000001), "xorl memory boundary: %#x", m32);
    TEST_ASSERT(m64 == UINT64_C(0xffffffff80000000),
                "xorq sign-extended imm32 boundary: %#" PRIx64, m64);
    TEST_ASSERT(flags & SF_FLAG, "xorq negative result: SF set");
    TEST_ASSERT(!(flags & ZF_FLAG), "xorq nonzero result: ZF clear");
    TEST_ASSERT(flags & PF_FLAG, "xorq low byte zero: PF set");
    TEST_ASSERT(!(flags & CF_FLAG), "xorq clears CF");
    TEST_ASSERT(!(flags & OF_FLAG), "xorq clears OF");

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "xorl $0x7fffffff, %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result) : : "rax", "cc"
    );
    TEST_ASSERT(result == UINT64_C(0x0000000080000000),
                "xorl writes eax and clears RAX high half: %#" PRIx64, result);
}

int main(void) {
    TEST_START("XOR instruction");
    test_xor_reg_reg();
    test_xor_reg_imm();
    test_xor_sizes();
    test_xor_width_memory_immediate_and_flags();
    TEST_END();
}
