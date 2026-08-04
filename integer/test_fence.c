/*
 * test_fence.c - Test x86-64 LFENCE/MFENCE/SFENCE instructions
 *
 * LFENCE: Serializes load operations (load fence).
 * MFENCE: Serializes all memory operations (full fence).
 * SFENCE: Serializes store operations (store fence).
 * These do not affect registers or flags. We verify they execute without faulting.
 *
 * Compile: gcc -o test_fence integer/test_fence.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_lfence(void) {
    volatile uint64_t data = 42;

    __asm__ volatile ("lfence" ::: "memory");
    TEST_ASSERT(data == 42, "lfence: executes without fault");
}

static void test_mfence(void) {
    volatile uint64_t data = 99;

    data = 100;
    __asm__ volatile ("mfence" ::: "memory");
    TEST_ASSERT(data == 100, "mfence: store visible after fence");
}

static void test_sfence(void) {
    volatile uint64_t data = 0;

    data = 77;
    __asm__ volatile ("sfence" ::: "memory");
    TEST_ASSERT(data == 77, "sfence: store visible after fence");
}

static void test_fence_sequence(void) {
    volatile uint64_t a = 0, b = 0;

    a = 1;
    __asm__ volatile ("sfence" ::: "memory");
    b = 2;
    __asm__ volatile ("mfence" ::: "memory");

    TEST_ASSERT(a == 1 && b == 2, "fence sequence: both stores visible");
}

static void test_fence_no_flags(void) {
    uint64_t flags_before, flags_after;

    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "lfence\n\t"
        "mfence\n\t"
        "sfence\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        :
        : "memory"
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "fences: flags unchanged");
}

static void test_fence_encodings_and_registers(void) {
    const unsigned char *bytes;
    uint64_t value = UINT64_C(0x0123456789abcdef);
    __asm__ volatile (
        "jmp 1f\n\t"
        "0: lfence\n\t" "mfence\n\t" "sfence\n\t"
        "1: leaq 0b(%%rip), %0\n\t"
        "lfence\n\t" "mfence\n\t" "sfence"
        : "=r"(bytes), "+r"(value)
        :
        : "memory");
    static const unsigned char expected[] = {
        0x0f, 0xae, 0xe8, 0x0f, 0xae, 0xf0, 0x0f, 0xae, 0xf8
    };
    TEST_ASSERT(memcmp(bytes, expected, sizeof(expected)) == 0,
                "LFENCE/MFENCE/SFENCE exact encodings");
    TEST_ASSERT(value == UINT64_C(0x0123456789abcdef),
                "Fence sequence preserves general-purpose registers");
}

int main(void) {
    TEST_START("LFENCE/MFENCE/SFENCE instructions");
    test_lfence();
    test_mfence();
    test_sfence();
    test_fence_sequence();
    test_fence_no_flags();
    test_fence_encodings_and_registers();
    TEST_END();
}
