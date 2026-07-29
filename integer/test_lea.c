/*
 * test_lea.c - Test x86-64 LEA instruction
 *
 * LEA computes effective address without accessing memory: DEST = effective_address
 * Does not affect any flags.
 * Common uses: address calculation, simple arithmetic without flag changes
 *
 * Compile: gcc -o test_lea integer/test_lea.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_lea_base_disp(void) {
    uint64_t result;

    /* LEA rax, [rbx + 0x10] */
    __asm__ volatile (
        "movq $0x1000, %%rbx\n\t"
        "leaq 0x10(%%rbx), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 0x1010, "lea [rbx+0x10] expected 0x1010, got 0x%lx", result);
}

static void test_lea_base_index(void) {
    uint64_t result;

    /* LEA rax, [rbx + rcx] */
    __asm__ volatile (
        "movq $100, %%rbx\n\t"
        "movq $200, %%rcx\n\t"
        "leaq (%%rbx, %%rcx), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 300, "lea [rbx+rcx] expected 300, got %lu", result);
}

static void test_lea_scale(void) {
    uint64_t result;

    /* LEA rax, [rbx + rcx*4] */
    __asm__ volatile (
        "movq $1000, %%rbx\n\t"
        "movq $10, %%rcx\n\t"
        "leaq (%%rbx, %%rcx, 4), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 1040, "lea [rbx+rcx*4] expected 1040, got %lu", result);

    /* LEA rax, [rcx*8 + 0x100] */
    __asm__ volatile (
        "movq $5, %%rcx\n\t"
        "leaq 0x100(, %%rcx, 8), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 0x100 + 40, "lea [rcx*8+0x100] expected 296, got %lu", result);
}

static void test_lea_full(void) {
    uint64_t result;

    /* LEA rax, [rbx + rcx*2 + 0x20] */
    __asm__ volatile (
        "movq $100, %%rbx\n\t"
        "movq $50, %%rcx\n\t"
        "leaq 0x20(%%rbx, %%rcx, 2), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 100 + 100 + 32, "lea [rbx+rcx*2+0x20] expected 232, got %lu", result);
}

static void test_lea_arithmetic(void) {
    uint64_t result;

    /* LEA for multiply by 3: lea rax, [rbx + rbx*2] */
    __asm__ volatile (
        "movq $17, %%rbx\n\t"
        "leaq (%%rbx, %%rbx, 2), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 51, "lea multiply by 3: 17*3 = 51, got %lu", result);

    /* LEA for multiply by 5: lea rax, [rbx + rbx*4] */
    __asm__ volatile (
        "movq $13, %%rbx\n\t"
        "leaq (%%rbx, %%rbx, 4), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 65, "lea multiply by 5: 13*5 = 65, got %lu", result);

    /* LEA for multiply by 9: lea rax, [rbx + rbx*8] */
    __asm__ volatile (
        "movq $7, %%rbx\n\t"
        "leaq (%%rbx, %%rbx, 8), %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 63, "lea multiply by 9: 7*9 = 63, got %lu", result);
}

static void test_lea_32bit(void) {
    uint64_t result;

    /* 32-bit LEA zero-extends to 64 bits */
    __asm__ volatile (
        "movq $0xFFFFFFFF00000000, %%rax\n\t"
        "movq $100, %%rbx\n\t"
        "leal 50(%%ebx), %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rbx"
    );
    TEST_ASSERT(result == 150, "leal zero-extends: expected 150, got 0x%lx", result);
}

static void test_lea_no_flags(void) {
    uint64_t flags_before, flags_after;

    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"   /* set ZF */
        "pushfq\n\t"
        "popq %0\n\t"
        "leaq 1(%%rax), %%rbx\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        :
        : "rax", "rbx"
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "LEA should not change flags");
}

int main(void) {
    TEST_START("LEA instruction");
    test_lea_base_disp();
    test_lea_base_index();
    test_lea_scale();
    test_lea_full();
    test_lea_arithmetic();
    test_lea_32bit();
    test_lea_no_flags();
    TEST_END();
}
