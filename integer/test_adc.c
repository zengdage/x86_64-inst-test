/*
 * test_adc.c - Test x86-64 ADC instruction
 *
 * ADC performs addition with carry: DEST = DEST + SRC + CF
 * Affects flags: CF, PF, AF, ZF, SF, OF
 *
 * Compile: gcc -o test_adc integer/test_adc.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_adc_with_carry_clear(void) {
    uint64_t result;
    uint64_t flags;

    /* ADC with CF=0: behaves like ADD */
    __asm__ volatile (
        "clc\n\t"
        "movq $10, %%rax\n\t"
        "movq $20, %%rbx\n\t"
        "adcq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 30, "adcq CF=0: 10+20+0 expected 30, got %lu", result);
}

static void test_adc_with_carry_set(void) {
    uint64_t result;
    uint64_t flags;

    /* ADC with CF=1 */
    __asm__ volatile (
        "stc\n\t"
        "movq $10, %%rax\n\t"
        "movq $20, %%rbx\n\t"
        "adcq %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 31, "adcq CF=1: 10+20+1 expected 31, got %lu", result);
}

static void test_adc_chain(void) {
    /* 128-bit addition using ADC chain: (0xFFFFFFFFFFFFFFFF + 1) across two 64-bit regs */
    uint64_t lo, hi;

    __asm__ volatile (
        "movq $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
        "xorq %%rdx, %%rdx\n\t"
        "addq $1, %%rax\n\t"
        "adcq $0, %%rdx\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1"
        : "=r"(lo), "=r"(hi)
        :
        : "rax", "rdx", "cc"
    );
    TEST_ASSERT(lo == 0 && hi == 1, "128-bit add chain: lo=%lu hi=%lu", lo, hi);
}

static void test_adc_overflow(void) {
    uint64_t result;
    uint64_t flags;

    /* UINT64_MAX + 0 + CF=1 => carry out */
    __asm__ volatile (
        "stc\n\t"
        "movq $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
        "adcq $0, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "adcq UINT64_MAX+0+1 expected 0");
    TEST_ASSERT(flags & CF_FLAG, "adcq UINT64_MAX+0+1: CF should be set");
    TEST_ASSERT(flags & ZF_FLAG, "adcq UINT64_MAX+0+1: ZF should be set");
}

static void test_adc_sizes(void) {
    uint8_t r8;
    uint16_t r16;
    uint32_t r32;
    uint64_t flags;

    /* 8-bit */
    __asm__ volatile (
        "stc\n\t"
        "movb $0xFE, %%al\n\t"
        "adcb $0, %%al\n\t"
        "movb %%al, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r8), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0xFF, "adcb 0xFE+0+1 expected 0xFF, got 0x%x", r8);

    /* 16-bit */
    __asm__ volatile (
        "stc\n\t"
        "movw $0xFFFF, %%ax\n\t"
        "adcw $0, %%ax\n\t"
        "movw %%ax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r16), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 0, "adcw 0xFFFF+0+1 expected 0, got 0x%x", r16);
    TEST_ASSERT(flags & CF_FLAG, "adcw overflow: CF should be set");

    /* 32-bit */
    __asm__ volatile (
        "stc\n\t"
        "movl $100, %%eax\n\t"
        "adcl $50, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(r32), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 151, "adcl 100+50+1 expected 151, got %u", r32);
}

int main(void) {
    TEST_START("ADC instruction");
    test_adc_with_carry_clear();
    test_adc_with_carry_set();
    test_adc_chain();
    test_adc_overflow();
    test_adc_sizes();
    TEST_END();
}
