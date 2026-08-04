/*
 * test_shld_shrd.c - Test x86-64 SHLD/SHRD instructions
 *
 * SHLD: Double precision shift left. Shifts DEST left, filling vacated bits from SRC.
 * SHRD: Double precision shift right. Shifts DEST right, filling vacated bits from SRC.
 * Affects flags: CF, ZF, SF, PF, OF (OF only for 1-bit shifts)
 *
 * Compile: gcc -o test_shld_shrd integer/test_shld_shrd.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_shld_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* SHLD: shift dest left by 4, fill with bits from src */
    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "movq $0xFEDCBA9876543210, %%rbx\n\t"
        "shldq $4, %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x23456789ABCDEF0FUL,
                "shldq by 4: expected 0x23456789ABCDEF0F, got 0x%lx", result);

    /* SHLD by 0 => no change */
    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "movq $99, %%rbx\n\t"
        "shldq $0, %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 42, "shldq by 0: unchanged");
}

static void test_shrd_basic(void) {
    uint64_t result;
    uint64_t flags;

    /* SHRD: shift dest right by 4, fill with bits from src */
    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "movq $0xFEDCBA9876543210, %%rbx\n\t"
        "shrdq $4, %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x0123456789ABCDEFUL,
                "shrdq by 4: expected 0x0123456789ABCDEF, got 0x%lx", result);
}

static void test_shld_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "movl $0xABCDEF01, %%ecx\n\t"
        "shldl $8, %%ecx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x345678AB, "shldl by 8: expected 0x345678AB, got 0x%x", result);
}

static void test_shrd_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "movl $0xABCDEF01, %%ecx\n\t"
        "shrdl $8, %%ecx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x01123456, "shrdl by 8: expected 0x01123456, got 0x%x", result);
}

static void test_shld_cl(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0xFF00FF00FF00FF00, %%rax\n\t"
        "movq $0x00FF00FF00FF00FF, %%rbx\n\t"
        "movb $8, %%cl\n\t"
        "shldq %%cl, %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 0x00FF00FF00FF0000UL,
                "shldq by cl=8: expected 0x00FF00FF00FF0000, got 0x%lx", result);
}

static void test_shld_16bit(void) {
    uint16_t result;

    __asm__ volatile (
        "movw $0x1234, %%ax\n\t"
        "movw $0xABCD, %%bx\n\t"
        "shldw $4, %%bx, %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == 0x234A, "shldw by 4: expected 0x234A, got 0x%x", result);
}

static void test_double_shift_count_boundaries(void) {
    uint64_t result;
    uint64_t flags;
    uint32_t r32;

    /* 64-bit counts are masked to six bits, including CL counts. */
    __asm__ volatile (
        "stc\n\t"
        "movq $0x123456789abcdef0, %%rax\n\t"
        "movq $0xfedcba9876543210, %%rbx\n\t"
        "shldq $64, %%rbx, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(result == UINT64_C(0x123456789abcdef0),
                "shldq count=64: destination unchanged");
    TEST_ASSERT(flags & CF_FLAG, "shldq effective count 0: CF preserved");

    __asm__ volatile (
        "movq $0, %%rax\n\t"
        "movq $0x8000000000000000, %%rbx\n\t"
        "movb $65, %%cl\n\t"
        "shldq %%cl, %%rbx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx", "cc"
    );
    TEST_ASSERT(result == 1, "shldq CL=65: expected effective count 1");

    /* 32-bit counts use five bits, so 32 is zero and 33 is one. */
    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "movl $0xabcdef01, %%ebx\n\t"
        "shrdl $32, %%ebx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(r32 == UINT32_C(0x12345678), "shrdl count=32: destination unchanged");

    __asm__ volatile (
        "movl $0, %%eax\n\t"
        "movl $1, %%ebx\n\t"
        "shrdl $33, %%ebx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax", "rbx", "cc"
    );
    TEST_ASSERT(r32 == UINT32_C(0x80000000), "shrdl count=33: expected effective count 1");
}

int main(void) {
    TEST_START("SHLD/SHRD instructions");
    test_shld_basic();
    test_shrd_basic();
    test_shld_32bit();
    test_shrd_32bit();
    test_shld_cl();
    test_shld_16bit();
    test_double_shift_count_boundaries();
    TEST_END();
}
