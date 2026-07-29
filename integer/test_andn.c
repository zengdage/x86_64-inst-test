/*
 * test_andn.c - Test x86-64 ANDN instruction (BMI1)
 *
 * ANDN: Bitwise AND of inverted first operand with second operand.
 * ANDN dest, src1, src2: dest = (~src1) & src2
 * Affects flags: SF, ZF (CF and OF cleared)
 *
 * Compile: gcc -o test_andn integer/test_andn.c -O0 -mbmi
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_andn_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* ~0xFF & 0xFFFF = 0xFF00 */
    __asm__ volatile (
        "movq $0xFF, %%rax\n\t"
        "movq $0xFFFF, %%rbx\n\t"
        "andnq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xFF00, "andnq ~0xFF & 0xFFFF = 0xFF00, got 0x%lx", result);
    TEST_ASSERT(!(flags & CF_FLAG), "andnq: CF should be cleared");
    TEST_ASSERT(!(flags & OF_FLAG), "andnq: OF should be cleared");
}

static void test_andn_all_ones(void) {
    uint64_t result;
    uint64_t flags;

    /* ~all_ones & anything = 0 */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "movq $0x12345678, %%rbx\n\t"
        "andnq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0, "andnq ~(-1) & x = 0");
    TEST_ASSERT(flags & ZF_FLAG, "andnq result=0: ZF should be set");
}

static void test_andn_zero(void) {
    uint64_t result;

    /* ~0 & anything = anything */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "movq $0xDEADBEEF, %%rbx\n\t"
        "andnq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xDEADBEEF, "andnq ~0 & 0xDEADBEEF = 0xDEADBEEF");
}

static void test_andn_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0x0F0F0F0F, %%eax\n\t"
        "movl $0xFFFFFFFF, %%ebx\n\t"
        "andnl %%ebx, %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xF0F0F0F0, "andnl ~0x0F0F0F0F & 0xFFFFFFFF = 0xF0F0F0F0, got 0x%x", result);
}

static void test_andn_sf(void) {
    uint64_t flags;

    /* Result with MSB set => SF */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "movq $0x8000000000000000, %%rbx\n\t"
        "andnq %%rbx, %%rax, %%rcx\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(flags & SF_FLAG, "andnq: SF should be set when MSB is set");
}

int main(void) {
    TEST_START("ANDN instruction (BMI1)");
    test_andn_basic();
    test_andn_all_ones();
    test_andn_zero();
    test_andn_32bit();
    test_andn_sf();
    TEST_END();
}
