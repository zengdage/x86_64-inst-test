/*
 * test_bswap.c - Test x86-64 BSWAP instruction
 *
 * BSWAP reverses the byte order of a 32-bit or 64-bit register.
 * Does not affect any flags.
 *
 * Compile: gcc -o test_bswap integer/test_bswap.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_bswap_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0x01020304, %%eax\n\t"
        "bswapl %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0x04030201, "bswapl 0x01020304 expected 0x04030201, got 0x%08x", result);

    __asm__ volatile (
        "movl $0xDEADBEEF, %%eax\n\t"
        "bswapl %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0xEFBEADDE, "bswapl 0xDEADBEEF expected 0xEFBEADDE, got 0x%08x", result);

    /* Double bswap = identity */
    __asm__ volatile (
        "movl $0x12345678, %%eax\n\t"
        "bswapl %%eax\n\t"
        "bswapl %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0x12345678, "bswapl double: identity");
}

static void test_bswap_64bit(void) {
    uint64_t result;

    __asm__ volatile (
        "movabsq $0x0102030405060708, %%rax\n\t"
        "bswapq %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0x0807060504030201UL,
                "bswapq 0x0102030405060708 expected 0x0807060504030201");

    /* All zeros */
    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "bswapq %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0, "bswapq 0 expected 0");

    /* All ones */
    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "bswapq %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 0xFFFFFFFFFFFFFFFFUL, "bswapq all-ones: unchanged");
}

int main(void) {
    TEST_START("BSWAP instruction");
    test_bswap_32bit();
    test_bswap_64bit();
    TEST_END();
}
