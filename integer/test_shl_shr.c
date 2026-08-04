/*
 * test_shl_shr.c - Test x86-64 SHL/SHR/SAL/SAR instructions
 *
 * SHL/SAL: Shift left (multiply by 2). CF = last bit shifted out.
 * SHR: Logical shift right (unsigned divide by 2). CF = last bit shifted out.
 * SAR: Arithmetic shift right (signed divide by 2). Preserves sign bit.
 * Affects flags: CF, ZF, SF, PF, OF (OF only defined for 1-bit shifts)
 *
 * Compile: gcc -o test_shl_shr integer/test_shl_shr.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_shl(void) {
    uint64_t result;
    uint64_t flags;

    /* SHL by 1 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "shlq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 2, "shlq 1<<1 expected 2");
    TEST_ASSERT(!(flags & CF_FLAG), "shlq 1<<1: CF clear");

    /* SHL by 63 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "shlq $63, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x8000000000000000UL, "shlq 1<<63");

    /* SHL with CF: shift out the MSB */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "shlq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "shlq MSB<<1 expected 0");
    TEST_ASSERT(flags & CF_FLAG, "shlq MSB<<1: CF should be set");

    /* SHL by CL */
    __asm__ volatile (
        "movq $0xFF, %%rax\n\t"
        "movb $4, %%cl\n\t"
        "shlq %%cl, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 0xFF0, "shlq 0xFF<<4 expected 0xFF0");
}

static void test_shr(void) {
    uint64_t result;
    uint64_t flags;

    /* SHR by 1 */
    __asm__ volatile (
        "movq $0x100, %%rax\n\t"
        "shrq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x80, "shrq 0x100>>1 expected 0x80");
    TEST_ASSERT(!(flags & CF_FLAG), "shrq 0x100>>1: CF clear");

    /* SHR with CF: shift out bit 0 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "shrq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "shrq 1>>1 expected 0");
    TEST_ASSERT(flags & CF_FLAG, "shrq 1>>1: CF should be set");
    TEST_ASSERT(flags & ZF_FLAG, "shrq 1>>1: ZF should be set");

    /* SHR of negative => clears sign bit */
    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "shrq $1, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x4000000000000000UL, "shrq sign bit>>1: unsigned shift");
}

static void test_sar(void) {
    int64_t result;
    uint64_t flags;

    /* SAR preserves sign */
    __asm__ volatile (
        "movq $-16, %%rax\n\t"
        "sarq $2, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == -4, "sarq -16>>2 expected -4, got %ld", result);
    TEST_ASSERT(flags & SF_FLAG, "sarq negative: SF should be set");

    /* SAR of positive */
    __asm__ volatile (
        "movq $64, %%rax\n\t"
        "sarq $3, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 8, "sarq 64>>3 expected 8");

    /* SAR of -1 => -1 (all ones remains all ones) */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "sarq $10, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == -1, "sarq -1>>10 expected -1");
}

static void test_shift_sizes(void) {
    uint8_t r8;
    uint16_t r16;
    uint32_t r32;

    __asm__ volatile (
        "movb $0x01, %%al\n\t"
        "shlb $7, %%al\n\t"
        "movb %%al, %0"
        : "=r"(r8)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0x80, "shlb 1<<7 expected 0x80");

    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "shrw $15, %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(r16)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 1, "shrw 0x8000>>15 expected 1");

    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "sarl $31, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 0xFFFFFFFF, "sarl 0x80000000>>31 expected all-ones (sign extension)");
}

static void test_shift_zero(void) {
    uint64_t result;

    /* Shift by 0 => no change */
    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "shlq $0, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 42, "shlq by 0: unchanged");
}

static void test_shift_count_boundaries(void) {
    uint64_t result;
    uint64_t flags;
    uint8_t r8;

    /* 64-bit counts are masked to six bits. */
    __asm__ volatile (
        "stc\n\t"
        "movq $0x1234, %%rax\n\t"
        "shlq $64, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == UINT64_C(0x1234), "shlq count=64: value unchanged");
    TEST_ASSERT(flags & CF_FLAG, "shlq effective count 0: CF preserved");

    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "shlq $65, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 2, "shlq count=65: expected effective count 1");

    __asm__ volatile (
        "movq $0x8000000000000000, %%rax\n\t"
        "movb $0xff, %%cl\n\t"
        "shrq %%cl, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 1, "shrq CL=255: expected effective count 63");

    /* For byte operands, count 8 is not reduced modulo the operand width. */
    __asm__ volatile (
        "movb $0xff, %%al\n\t"
        "shlb $8, %%al\n\t"
        "movb %%al, %0"
        : "=r"(r8)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0, "shlb count=8: all bits shifted out");
}

int main(void) {
    TEST_START("SHL/SHR/SAL/SAR instructions");
    test_shl();
    test_shr();
    test_sar();
    test_shift_sizes();
    test_shift_zero();
    test_shift_count_boundaries();
    TEST_END();
}
