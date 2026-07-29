/*
 * test_bextr.c - Test x86-64 BEXTR instruction (BMI1)
 *
 * BEXTR: Bit field extract. Extracts contiguous bits from source.
 * BEXTR dest, src, ctrl where ctrl[7:0]=start, ctrl[15:8]=length
 * Affects flags: ZF (set if result is 0); CF, OF cleared; SF, PF, AF undefined.
 *
 * Compile: gcc -o test_bextr integer/test_bextr.c -O0 -mbmi
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_bextr_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* Extract 4 bits starting at bit 4 from 0xFF */
    /* ctrl = (4 << 8) | 4 = 0x0404 */
    __asm__ volatile (
        "movq $0xFF, %%rax\n\t"
        "movq $0x0404, %%rbx\n\t"
        "bextrq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xF, "bextrq 0xFF start=4 len=4: expected 0xF, got 0x%lx", result);
    TEST_ASSERT(!(flags & ZF_FLAG), "bextrq: ZF should be clear");
}

static void test_bextr_full(void) {
    uint64_t result;

    /* Extract full 64 bits (start=0, len=64) */
    /* But len is 8-bit field, max meaningful = 64 */
    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "movq $0x4000, %%rbx\n\t"     /* start=0, len=64 */
        "bextrq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x123456789ABCDEF0UL, "bextrq start=0 len=64: full value");
}

static void test_bextr_zero_len(void) {
    uint64_t result;
    uint64_t flags;

    /* Length 0 => result is 0 */
    __asm__ volatile (
        "movq $0xFFFFFFFF, %%rax\n\t"
        "movq $0x0004, %%rbx\n\t"     /* start=4, len=0 */
        "bextrq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0, "bextrq len=0: expected 0");
    TEST_ASSERT(flags & ZF_FLAG, "bextrq len=0: ZF should be set");
}

static void test_bextr_32bit(void) {
    uint32_t result;

    /* Extract bits [8:15] from 0xABCD1234 */
    /* start=8, len=8 => ctrl = 0x0808 */
    __asm__ volatile (
        "movl $0xABCD1234, %%eax\n\t"
        "movl $0x0808, %%ebx\n\t"
        "bextrl %%ebx, %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x12, "bextrl bits[8:15] of 0xABCD1234: expected 0x12, got 0x%x", result);
}

static void test_bextr_high_bits(void) {
    uint64_t result;

    /* Extract bits [60:63] from 0xF000000000000000 */
    /* start=60, len=4 => ctrl = 0x043C */
    __asm__ volatile (
        "movabsq $0xF000000000000000, %%rax\n\t"
        "movq $0x043C, %%rbx\n\t"
        "bextrq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xF, "bextrq bits[60:63]: expected 0xF, got 0x%lx", result);
}

int main(void) {
    TEST_START("BEXTR instruction (BMI1)");
    test_bextr_basic();
    test_bextr_full();
    test_bextr_zero_len();
    test_bextr_32bit();
    test_bextr_high_bits();
    TEST_END();
}
