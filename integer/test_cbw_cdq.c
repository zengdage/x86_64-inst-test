/*
 * test_cbw_cdq.c - Test x86-64 CBW/CWDE/CDQE/CWD/CDQ/CQO instructions
 *
 * CBW:  Sign-extend AL to AX
 * CWDE: Sign-extend AX to EAX
 * CDQE: Sign-extend EAX to RAX
 * CWD:  Sign-extend AX to DX:AX
 * CDQ:  Sign-extend EAX to EDX:EAX
 * CQO:  Sign-extend RAX to RDX:RAX
 * None of these affect flags.
 *
 * Compile: gcc -o test_cbw_cdq integer/test_cbw_cdq.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_cbw(void) {
    uint16_t result;

    /* Positive: AL=0x7F => AX=0x007F */
    __asm__ volatile (
        "movb $0x7F, %%al\n\t"
        "cbw\n\t"
        "movw %%ax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0x007F, "cbw 0x7F: expected 0x007F, got 0x%04x", result);

    /* Negative: AL=0x80 => AX=0xFF80 */
    __asm__ volatile (
        "movb $0x80, %%al\n\t"
        "cbw\n\t"
        "movw %%ax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0xFF80, "cbw 0x80: expected 0xFF80, got 0x%04x", result);
}

static void test_cwde(void) {
    uint32_t result;

    /* Positive */
    __asm__ volatile (
        "movw $0x7FFF, %%ax\n\t"
        "cwde\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0x00007FFF, "cwde 0x7FFF: expected 0x00007FFF");

    /* Negative */
    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "cwde\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0xFFFF8000, "cwde 0x8000: expected 0xFFFF8000, got 0x%08x", result);
}

static void test_cdqe(void) {
    uint64_t result;

    /* Positive */
    __asm__ volatile (
        "movl $0x7FFFFFFF, %%eax\n\t"
        "cdqe\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0x7FFFFFFF, "cdqe 0x7FFFFFFF: positive");

    /* Negative */
    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "cdqe\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0xFFFFFFFF80000000UL, "cdqe 0x80000000: sign-extended");
}

static void test_cwd(void) {
    uint16_t dx_val, ax_val;

    /* Positive */
    __asm__ volatile (
        "movw $0x1234, %%ax\n\t"
        "cwd\n\t"
        "movw %%dx, %0\n\t"
        "movw %%ax, %1"
        : "=r"(dx_val), "=r"(ax_val)
        :
        : "rax", "rdx"
    );
    TEST_ASSERT(dx_val == 0, "cwd 0x1234: DX should be 0");
    TEST_ASSERT(ax_val == 0x1234, "cwd 0x1234: AX unchanged");

    /* Negative */
    __asm__ volatile (
        "movw $0x8000, %%ax\n\t"
        "cwd\n\t"
        "movw %%dx, %0"
        : "=r"(dx_val)
        :
        : "rax", "rdx"
    );
    TEST_ASSERT(dx_val == 0xFFFF, "cwd 0x8000: DX should be 0xFFFF");
}

static void test_cdq(void) {
    uint32_t edx_val;

    /* Positive */
    __asm__ volatile (
        "movl $100, %%eax\n\t"
        "cdq\n\t"
        "movl %%edx, %0"
        : "=r"(edx_val)
        :
        : "rax", "rdx"
    );
    TEST_ASSERT(edx_val == 0, "cdq positive: EDX should be 0");

    /* Negative */
    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "cdq\n\t"
        "movl %%edx, %0"
        : "=r"(edx_val)
        :
        : "rax", "rdx"
    );
    TEST_ASSERT(edx_val == 0xFFFFFFFF, "cdq negative: EDX should be 0xFFFFFFFF");
}

static void test_cqo(void) {
    uint64_t rdx_val;

    /* Positive */
    __asm__ volatile (
        "movq $100, %%rax\n\t"
        "cqo\n\t"
        "movq %%rdx, %0"
        : "=r"(rdx_val)
        :
        : "rax", "rdx"
    );
    TEST_ASSERT(rdx_val == 0, "cqo positive: RDX should be 0");

    /* Negative */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "cqo\n\t"
        "movq %%rdx, %0"
        : "=r"(rdx_val)
        :
        : "rax", "rdx"
    );
    TEST_ASSERT(rdx_val == 0xFFFFFFFFFFFFFFFFUL, "cqo negative: RDX should be all ones");
}

int main(void) {
    TEST_START("CBW/CWDE/CDQE/CWD/CDQ/CQO instructions");
    test_cbw();
    test_cwde();
    test_cdqe();
    test_cwd();
    test_cdq();
    test_cqo();
    TEST_END();
}
