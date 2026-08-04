/*
 * test_rol_ror.c - Test x86-64 ROL/ROR/RCL/RCR instructions
 *
 * ROL: Rotate left. CF = last bit rotated.
 * ROR: Rotate right. CF = last bit rotated.
 * RCL: Rotate left through carry (includes CF in rotation).
 * RCR: Rotate right through carry (includes CF in rotation).
 * Affects flags: CF, OF (OF only for 1-bit rotates)
 *
 * Compile: gcc -o test_rol_ror integer/test_rol_ror.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_rol(void) {
    uint64_t result;
    uint64_t flags;

    /* ROL by 1 */
    __asm__ volatile (
        "movq $0x8000000000000001, %%rax\n\t"
        "rolq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 3, "rolq 0x8000000000000001 by 1: expected 3, got 0x%lx", result);
    TEST_ASSERT(flags & CF_FLAG, "rolq: CF = rotated-out MSB = 1");

    /* ROL by 4 */
    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "rolq $4, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x23456789ABCDEF01UL, "rolq by 4: nibble rotate");

    /* Full rotation (64) = identity */
    __asm__ volatile (
        "movq $0xDEADBEEF, %%rax\n\t"
        "rolq $64, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0xDEADBEEF, "rolq by 64: identity");
}

static void test_ror(void) {
    uint64_t result;
    uint64_t flags;

    /* ROR by 1 */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "rorq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x8000000000000000UL, "rorq 1 by 1: bit wraps to MSB");
    TEST_ASSERT(flags & CF_FLAG, "rorq 1 by 1: CF = rotated-out bit = 1");

    /* ROR by 4 */
    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "rorq $4, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x0123456789ABCDEFUL, "rorq by 4: nibble rotate right");
}

static void test_rcl(void) {
    uint64_t result;
    uint64_t flags;

    /* RCL by 1 with CF=1: rotates CF into bit 0 */
    __asm__ volatile (
        "stc\n\t"
        "movq $0, %%rax\n\t"
        "rclq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 1, "rclq CF=1, 0 by 1: expected 1");
    TEST_ASSERT(!(flags & CF_FLAG), "rclq: CF should be 0 (old MSB was 0)");

    /* RCL: MSB goes to CF */
    __asm__ volatile (
        "clc\n\t"
        "movq $0x8000000000000000, %%rax\n\t"
        "rclq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0, "rclq 0x80..0 CF=0 by 1: expected 0");
    TEST_ASSERT(flags & CF_FLAG, "rclq: CF should be set (old MSB was 1)");
}

static void test_rcr(void) {
    uint64_t result;
    uint64_t flags;

    /* RCR by 1 with CF=1: CF goes to MSB */
    __asm__ volatile (
        "stc\n\t"
        "movq $0, %%rax\n\t"
        "rcrq $1, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 0x8000000000000000UL, "rcrq CF=1, 0 by 1: CF goes to MSB");
    TEST_ASSERT(!(flags & CF_FLAG), "rcrq: CF = old bit 0 = 0");
}

static void test_rotate_sizes(void) {
    uint32_t r32;
    uint16_t r16;
    uint8_t r8;

    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "roll $8, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(r32)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r32 == 0x34567812, "roll by 8: byte rotate");

    __asm__ volatile (
        "movw $0x1234, %%ax\n\t"
        "rorw $8, %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(r16)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r16 == 0x3412, "rorw by 8: byte swap");

    __asm__ volatile (
        "movb $0x81, %%al\n\t"
        "rolb $1, %%al\n\t"
        "movb %%al, %0"
        : "=r"(r8)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(r8 == 0x03, "rolb 0x81 by 1: expected 0x03");
}

static void test_rotate_count_boundaries(void) {
    uint64_t result;
    uint64_t flags;

    /* 64-bit rotate counts use only the low six bits: 65 becomes 1. */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "rolq $65, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 2, "rolq count=65: expected effective count 1");

    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "rorq $255, %%rax\n\t"     /* 255 & 63 = 63 */
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == 2, "rorq count=255: expected effective count 63");

    /* An effective zero count must preserve both the value and flags. */
    __asm__ volatile (
        "stc\n\t"
        "movq $0x12345678, %%rax\n\t"
        "rolq $64, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(result), "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(result == UINT64_C(0x12345678), "rolq count=64: value unchanged");
    TEST_ASSERT(flags & CF_FLAG, "rolq effective count 0: CF preserved");

    /* The CL form follows the same count masking rule. */
    __asm__ volatile (
        "stc\n\t"
        "movq $0, %%rax\n\t"
        "movb $65, %%cl\n\t"
        "rclq %%cl, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rcx", "cc"
    );
    TEST_ASSERT(result == 1, "rclq CL=65: expected effective count 1");
}

int main(void) {
    TEST_START("ROL/ROR/RCL/RCR instructions");
    test_rol();
    test_ror();
    test_rcl();
    test_rcr();
    test_rotate_sizes();
    test_rotate_count_boundaries();
    TEST_END();
}
