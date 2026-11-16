#include "../common.h"

__attribute__((naked, noinline))
static void execute_leave(uint64_t *result, uint64_t *frame) {
    __asm__ volatile (
        "movq %rsp, %r8\n\t"
        "movq %rbp, %r9\n\t"
        "movq %rsi, %rbp\n\t"
        "leave\n\t"
        "movq %rbp, 0(%rdi)\n\t"
        "movq %rsp, 8(%rdi)\n\t"
        "movq %r8, %rsp\n\t"
        "movq %r9, %rbp\n\t"
        "ret");
}

static void test_leave(void) {
    uint64_t frame[2] = {UINT64_C(0x123456789abcdef0), 0};
    uint64_t result[2] = {0, 0};
    execute_leave(result, frame);
    TEST_ASSERT(result[0] == frame[0], "leave pops RBP from the frame");
    TEST_ASSERT(result[1] == (uintptr_t)&frame[1], "leave advances RSP by eight bytes");
}

static void test_xlatb(void) {
    uint8_t table[256];
    for (unsigned i = 0; i < 256; ++i)
        table[i] = (uint8_t)(i ^ 0xa5);

    static const unsigned indexes[] = {0, 1, 127, 255};
    for (unsigned i = 0; i < sizeof(indexes) / sizeof(indexes[0]); ++i) {
        uint64_t result, before, after;
        __asm__ volatile (
            "movq $0x8d5, %%r11\n\tpushq %%r11\n\tpopfq\n\t"
            "pushfq\n\tpopq %[before]\n\t"
            "xlatb\n\t"
            "pushfq\n\tpopq %[after]"
            : "=a"(result), [before] "=&r"(before), [after] "=&r"(after)
            : "0"(UINT64_C(0x1122334455667700) | indexes[i]), "b"(table)
            : "r11", "cc", "memory");
        TEST_ASSERT(result == (UINT64_C(0x1122334455667700) | table[indexes[i]]),
                    "xlatb unsigned index %u and preserves upper RAX", indexes[i]);
        TEST_ASSERT((before & 0x8d5) == (after & 0x8d5),
                    "xlatb index %u preserves status flags", indexes[i]);
    }
}

int main(void) {
    TEST_START("LEAVE/XLATB");
    test_leave();
    test_xlatb();
    TEST_END();
}
