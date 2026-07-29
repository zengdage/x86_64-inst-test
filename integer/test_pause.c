/*
 * test_pause.c - Test x86-64 PAUSE instruction
 *
 * PAUSE provides a hint to the processor that the current code is in a
 * spin-wait loop. It improves power efficiency and performance of spin loops.
 * Does not affect registers or flags. Encoding: F3 90 (REP NOP).
 *
 * Compile: gcc -o test_pause integer/test_pause.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pause_basic(void) {
    uint64_t before, after;

    __asm__ volatile (
        "movq $42, %%rax\n\t"
        "movq %%rax, %0\n\t"
        "pause\n\t"
        "movq %%rax, %1"
        : "=r"(before), "=r"(after)
        :
        : "rax"
    );
    TEST_ASSERT(before == after, "pause: register unchanged");
}

static void test_pause_no_flags(void) {
    uint64_t flags_before, flags_after;

    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "pause\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        :
        : "cc"
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "pause: flags unchanged");
}

static void test_pause_in_loop(void) {
    /* Common pattern: spin-wait loop with PAUSE */
    volatile int flag = 0;
    uint64_t iterations = 0;

    /* Simulate: spin until flag is set (set it after a few iterations) */
    __asm__ volatile (
        "movl $0, %0\n\t"
        "1:\n\t"
        "addq $1, %1\n\t"
        "cmpq $100, %1\n\t"
        "jge 2f\n\t"
        "pause\n\t"
        "jmp 1b\n\t"
        "2:"
        : "+m"(flag), "+r"(iterations)
        :
        : "cc"
    );
    TEST_ASSERT(iterations == 100, "pause in loop: 100 iterations, got %lu", iterations);
}

static void test_pause_multiple(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $99, %%rax\n\t"
        "pause\n\t"
        "pause\n\t"
        "pause\n\t"
        "pause\n\t"
        "movq %%rax, %0"
        : "=r"(result)
        :
        : "rax"
    );
    TEST_ASSERT(result == 99, "multiple pauses: register unchanged");
}

static void test_pause_encoding(void) {
    /* PAUSE is encoded as F3 90 (REP NOP) */
    unsigned char bytes[2];
    __asm__ volatile (
        "jmp 1f\n\t"
        "0: pause\n\t"
        "1:\n\t"
        "leaq 0b(%%rip), %%rax\n\t"
        "movb (%%rax), %%cl\n\t"
        "movb %%cl, %0\n\t"
        "movb 1(%%rax), %%cl\n\t"
        "movb %%cl, %1"
        : "=r"(bytes[0]), "=r"(bytes[1])
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(bytes[0] == 0xF3 && bytes[1] == 0x90,
                "pause encoding: expected F3 90, got %02x %02x", bytes[0], bytes[1]);
}

int main(void) {
    TEST_START("PAUSE instruction");
    test_pause_basic();
    test_pause_no_flags();
    test_pause_in_loop();
    test_pause_multiple();
    test_pause_encoding();
    TEST_END();
}
