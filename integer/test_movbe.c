/*
 * test_movbe.c - Test x86-64 MOVBE instruction
 *
 * MOVBE loads/stores with byte-swap (converts between big-endian and little-endian).
 * MOVBE r, m: Load from memory with byte swap
 * MOVBE m, r: Store to memory with byte swap
 * Does not affect flags.
 * Requires MOVBE CPU support.
 *
 * Compile: gcc -o test_movbe integer/test_movbe.c -O0 -mmovbe
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movbe_load_32(void) {
    uint32_t src = 0x01020304;
    uint32_t result;

    __asm__ volatile (
        "movbe %1, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(result)
        : "m"(src)
        : "rax"
    );
    TEST_ASSERT(result == 0x04030201, "movbe load 32: 0x01020304 => 0x%08x", result);
}

static void test_movbe_load_64(void) {
    uint64_t src = 0x0102030405060708UL;
    uint64_t result;

    __asm__ volatile (
        "movbe %1, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        : "m"(src)
        : "rax"
    );
    TEST_ASSERT(result == 0x0807060504030201UL, "movbe load 64: byte-swapped");
}

static void test_movbe_store_32(void) {
    uint32_t dst;

    __asm__ volatile (
        "movl $0x01020304, %%eax\n\t"
        "movbe %%eax, %0"
        : "=m"(dst)
        :
        : "rax"
    );
    TEST_ASSERT(dst == 0x04030201, "movbe store 32: 0x01020304 => 0x%08x", dst);
}

static void test_movbe_store_64(void) {
    uint64_t dst;

    __asm__ volatile (
        "movabsq $0x0102030405060708, %%rax\n\t"
        "movbe %%rax, %0"
        : "=m"(dst)
        :
        : "rax"
    );
    TEST_ASSERT(dst == 0x0807060504030201UL, "movbe store 64: byte-swapped");
}

static void test_movbe_roundtrip(void) {
    uint64_t val = 0xDEADBEEFCAFEBABEUL;
    uint64_t result;
    uint64_t tmp;

    /* Store with byte swap, then load with byte swap => original value */
    __asm__ volatile (
        "movq %2, %%rax\n\t"
        "movbe %%rax, %1\n\t"
        "movbe %1, %%rax\n\t"
        "movq %%rax, %0"
        : "=r"(result), "=m"(tmp)
        : "r"(val)
        : "rax"
    );
    TEST_ASSERT(result == val, "movbe roundtrip: double swap = identity");
}

static void test_movbe_load_16(void) {
    uint16_t src = 0x0102;
    uint16_t result;

    __asm__ volatile (
        "movbe %1, %%ax\n\t"
        "movw %%ax, %0"
        : "=r"(result)
        : "m"(src)
        : "rax"
    );
    TEST_ASSERT(result == 0x0201, "movbe load 16: 0x0102 => 0x%04x", result);
}

int main(void) {
    TEST_START("MOVBE instruction");
    test_movbe_load_32();
    test_movbe_load_64();
    test_movbe_store_32();
    test_movbe_store_64();
    test_movbe_roundtrip();
    test_movbe_load_16();
    TEST_END();
}
