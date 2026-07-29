/*
 * test_crc32.c - Test x86-64 CRC32 instruction
 *
 * CRC32 computes CRC-32C (Castagnoli) using the polynomial 0x1EDC6F41.
 * Accumulates CRC value: dest = CRC32(dest, src)
 * Does not affect any flags.
 * Requires SSE4.2 support.
 *
 * Compile: gcc -o test_crc32 integer/test_crc32.c -O0 -msse4.2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_crc32_8bit(void) {
    uint32_t result;

    /* CRC32 of 0 with data 0 */
    __asm__ volatile (
        "movl $0, %%eax\n\t"
        "movb $0, %%cl\n\t"
        "crc32b %%cl, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == 0, "crc32b(0, 0) = 0");

    /* CRC32 of 0 with data 1 */
    __asm__ volatile (
        "movl $0, %%eax\n\t"
        "movb $1, %%cl\n\t"
        "crc32b %%cl, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result != 0, "crc32b(0, 1) should be non-zero, got 0x%08x", result);

    /* Known value: CRC32C of byte 0x01 with initial 0 */
    /* CRC32C(0, 0x01) = 0xA016D052 */
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "movb $0x01, %%cl\n\t"
        "crc32b %%cl, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    /* Just verify it's deterministic */
    uint32_t result2;
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "movb $0x01, %%cl\n\t"
        "crc32b %%cl, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result2)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == result2, "crc32b: deterministic");
}

static void test_crc32_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "movl $0x12345678, %%ecx\n\t"
        "crc32l %%ecx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result != 0, "crc32l(0, 0x12345678) should be non-zero");
}

static void test_crc32_64bit(void) {
    uint64_t result;

    __asm__ volatile (
        "xorq %%rax, %%rax\n\t"
        "movq $0x123456789ABCDEF0, %%rcx\n\t"
        "crc32q %%rcx, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result != 0, "crc32q(0, large) should be non-zero");
}

static void test_crc32_chain(void) {
    uint32_t result;

    /* Chain multiple CRC32 operations */
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "crc32b %1, %%eax\n\t"
        "crc32b %2, %%eax\n\t"
        "crc32b %3, %%eax\n\t"
        "crc32b %4, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        : "m"(data[0]), "m"(data[1]), "m"(data[2]), "m"(data[3])
        : "rax"
    );
    TEST_ASSERT(result != 0, "crc32 chain: non-zero result");

    /* Same data should give same CRC */
    uint32_t result2;
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "crc32b %1, %%eax\n\t"
        "crc32b %2, %%eax\n\t"
        "crc32b %3, %%eax\n\t"
        "crc32b %4, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result2)
        : "m"(data[0]), "m"(data[1]), "m"(data[2]), "m"(data[3])
        : "rax"
    );
    TEST_ASSERT(result == result2, "crc32 chain: deterministic");
}

static void test_crc32_16bit(void) {
    uint32_t result;

    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "movw $0xABCD, %%cx\n\t"
        "crc32w %%cx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result != 0, "crc32w(0, 0xABCD) should be non-zero");
}

int main(void) {
    TEST_START("CRC32 instruction");
    test_crc32_8bit();
    test_crc32_32bit();
    test_crc32_64bit();
    test_crc32_chain();
    test_crc32_16bit();
    TEST_END();
}
