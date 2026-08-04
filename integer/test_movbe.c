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

static void test_movbe_store_16_and_flags(void) {
    uint16_t dst = 0;
    uint32_t loaded;
    uint64_t before, after;
    uint32_t src = UINT32_C(0x01020304);
    __asm__ volatile (
        "movw $0x1234, %%ax\n\t" "movbe %%ax, %0\n\t"
        "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "pushfq\n\t" "popq %2\n\t" "movbe %4, %%eax\n\t"
        "pushfq\n\t" "popq %3\n\t" "movl %%eax, %1"
        : "=m"(dst), "=r"(loaded), "=&r"(before), "=&r"(after)
        : "m"(src)
        : "rax", "r11", "cc");
    TEST_ASSERT(dst == UINT16_C(0x3412), "movbe store16: got %#x", dst);
    TEST_ASSERT(loaded == UINT32_C(0x04030201), "movbe load used in flags test");
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((before & mask) == (after & mask), "movbe preserves status flags");
}

int main(void) {
    TEST_START("MOVBE instruction");
    test_movbe_load_32();
    test_movbe_load_64();
    test_movbe_store_32();
    test_movbe_store_64();
    test_movbe_roundtrip();
    test_movbe_load_16();
    test_movbe_store_16_and_flags();
    TEST_END();
}
