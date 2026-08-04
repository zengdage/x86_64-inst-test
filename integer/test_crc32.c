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

static uint32_t crc32c_byte(uint32_t crc, uint8_t value) {
    crc ^= value;
    for (int bit = 0; bit < 8; bit++)
        crc = (crc >> 1) ^ ((crc & 1U) ? UINT32_C(0x82f63b78) : 0U);
    return crc;
}

static uint32_t crc32c_le(uint32_t crc, uint64_t value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; i++) {
        crc = crc32c_byte(crc, (uint8_t)value);
        value >>= 8;
    }
    return crc;
}

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
    TEST_ASSERT(result == crc32c_le(0, 1, 1),
                "crc32b(0,1): got 0x%08x expected 0x%08x",
                result, crc32c_le(0, 1, 1));

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
    TEST_ASSERT(result == crc32c_le(0, 0x01, 1),
                "crc32b known answer: got 0x%08x expected 0x%08x",
                result, crc32c_le(0, 0x01, 1));

    /* Initial-CRC and data boundaries. */
    uint32_t initial = UINT32_MAX;
    uint8_t data = UINT8_MAX;
    __asm__ volatile ("crc32b %1, %0" : "+r"(initial) : "r"(data));
    TEST_ASSERT(initial == crc32c_le(UINT32_MAX, UINT8_MAX, 1),
                "crc32b(ffffffff,ff): got 0x%08x expected 0x%08x",
                initial, crc32c_le(UINT32_MAX, UINT8_MAX, 1));
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
    TEST_ASSERT(result == crc32c_le(0, UINT32_C(0x12345678), 4),
                "crc32l: got 0x%08x expected 0x%08x", result,
                crc32c_le(0, UINT32_C(0x12345678), 4));
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
    TEST_ASSERT(result == crc32c_le(0, UINT64_C(0x123456789abcdef0), 8),
                "crc32q: got 0x%016" PRIx64 " expected 0x%08x", result,
                crc32c_le(0, UINT64_C(0x123456789abcdef0), 8));
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
    uint32_t expected = 0;
    for (unsigned i = 0; i < sizeof(data); i++) expected = crc32c_byte(expected, data[i]);
    TEST_ASSERT(result == expected, "crc32 chain: got 0x%08x expected 0x%08x", result, expected);

    /* A dword source must consume the same bytes in little-endian order. */
    uint32_t result2;
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"
        "movl $0x04030201, %%ecx\n\t"
        "crc32l %%ecx, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result2)
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(result == result2, "crc32 byte chain equals crc32l little-endian source");
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
    TEST_ASSERT(result == crc32c_le(0, UINT16_C(0xabcd), 2),
                "crc32w: got 0x%08x expected 0x%08x", result,
                crc32c_le(0, UINT16_C(0xabcd), 2));
}

static void test_crc32_flags(void) {
    uint64_t before, after;
    uint32_t crc = UINT32_C(0x12345678);
    uint32_t data = UINT32_C(0x80000000);

    __asm__ volatile (
        "movq $0x8d5, %%r11\n\t"
        "pushq %%r11\n\t"
        "popfq\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "crc32l %3, %2\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=&r"(before), "=&r"(after), "+r"(crc)
        : "r"(data)
        : "r11", "cc"
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((before & mask) == (after & mask),
                "crc32 preserves flags: before=%#" PRIx64 " after=%#" PRIx64,
                before & mask, after & mask);
}

int main(void) {
    TEST_START("CRC32 instruction");
    test_crc32_8bit();
    test_crc32_32bit();
    test_crc32_64bit();
    test_crc32_chain();
    test_crc32_16bit();
    test_crc32_flags();
    TEST_END();
}
