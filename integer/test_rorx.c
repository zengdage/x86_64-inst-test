/*
 * test_rorx.c - Test x86-64 RORX instruction (BMI2)
 *
 * RORX: Rotate right without affecting flags. Uses VEX encoding.
 * RORX dest, src, imm8: dest = rotate_right(src, imm8)
 * Does not affect any flags (unlike ROR).
 *
 * Compile: gcc -o test_rorx integer/test_rorx.c -O0 -mbmi2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_rorx_64bit(void) {
    uint64_t result;

    /* Rotate right by 4 */
    __asm__ volatile (
        "movq $0x123456789ABCDEF0, %%rax\n\t"
        "rorxq $4, %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 0x0123456789ABCDEFUL, "rorxq by 4: nibble rotate right");

    /* Rotate right by 0 => identity */
    __asm__ volatile (
        "movq $0xDEADBEEF, %%rax\n\t"
        "rorxq $0, %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 0xDEADBEEF, "rorxq by 0: identity");

    /* Rotate right by 32 => swap halves */
    __asm__ volatile (
        "movabsq $0xAAAABBBBCCCCDDDD, %%rax\n\t"
        "rorxq $32, %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 0xCCCCDDDDAAAABBBBUL, "rorxq by 32: swap halves");
}

static void test_rorx_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "rorxl $8, %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 0x78123456, "rorxl by 8: expected 0x78123456, got 0x%x", result);

    __asm__ volatile (
        "movl $0x80000001, %%eax\n\t"
        "rorxl $1, %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 0xC0000000, "rorxl 0x80000001 by 1: expected 0xC0000000, got 0x%x", result);
}

static void test_rorx_no_flags(void) {
    uint64_t flags_before, flags_after;

    /* RORX should NOT affect flags */
    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "movq $0xFF, %%rax\n\t"
        "rorxq $4, %%rax, %%rbx\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        :
        : "rax", "rbx"
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "rorxq: flags should be unchanged");
}

static void test_rorx_mem(void) {
    uint64_t src = 0x0102030405060708UL;
    uint64_t result;

    __asm__ volatile (
        "rorxq $8, %1, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        : "m"(src)
        : "rax"
    );
    TEST_ASSERT(result == 0x0801020304050607UL, "rorxq mem by 8");
}

static void test_rorx_count_boundaries(void) {
    uint64_t result;
    uint32_t r32;

    __asm__ volatile (
        "movq $0x123456789abcdef0, %%rax\n\t"
        "rorxq $64, %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == UINT64_C(0x123456789abcdef0),
                "rorxq count=64: expected identity");

    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "rorxq $65, %%rax, %%rbx\n\t"
        "movq %%rbx, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == UINT64_C(0x8000000000000000),
                "rorxq count=65: expected effective count 1");

    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "rorxl $32, %%eax, %%ebx\n\t"
        "movl %%ebx, %0"
        : "=r"(r32)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(r32 == UINT32_C(0x12345678), "rorxl count=32: expected identity");

    __asm__ volatile (
        "movl $1, %%eax\n\t"
        "rorxl $255, %%eax, %%ebx\n\t"
        "movl %%ebx, %0"
        : "=r"(r32)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(r32 == 2, "rorxl count=255: expected effective count 31");
}

int main(void) {
    TEST_START("RORX instruction (BMI2)");
    test_rorx_64bit();
    test_rorx_32bit();
    test_rorx_no_flags();
    test_rorx_mem();
    test_rorx_count_boundaries();
    TEST_END();
}
