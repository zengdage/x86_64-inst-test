#include "../common.h"

#if !defined(__i386__)
#error "Build this test with -m32"
#endif

static void test_jcxz(void) {
    uint32_t taken;
    __asm__ volatile (
        "movl $0x12340000, %%ecx\n\tmovl $0, %0\n\tjcxz 1f\n\tjmp 2f\n\t"
        "1: movl $1, %0\n\t2:"
        : "=&r"(taken) : : "ecx", "cc");
    TEST_ASSERT(taken == 1, "jcxz tests CX and ignores high ECX bits");
    __asm__ volatile (
        "movl $0x00010000, %%ecx\n\tmovl $0, %0\n\tjecxz 1f\n\tjmp 2f\n\t"
        "1: movl $1, %0\n\t2:"
        : "=&r"(taken) : : "ecx", "cc");
    TEST_ASSERT(taken == 0, "jecxz tests the complete ECX value");
}

static void test_arpl(void) {
    uint16_t destination;
    uint8_t adjusted;
    destination = UINT16_C(0x0020);
    __asm__ volatile ("arpl %2, %0\n\tsetz %1"
                      : "+rm"(destination), "=qm"(adjusted)
                      : "r"((uint16_t)UINT16_C(0x0003)) : "cc");
    TEST_ASSERT(destination == UINT16_C(0x0023) && adjusted,
                "arpl raises destination RPL and sets ZF");
    destination = UINT16_C(0x0023);
    __asm__ volatile ("arpl %2, %0\n\tsetz %1"
                      : "+rm"(destination), "=qm"(adjusted)
                      : "r"((uint16_t)UINT16_C(0x0001)) : "cc");
    TEST_ASSERT(destination == UINT16_C(0x0023) && !adjusted,
                "arpl leaves sufficient destination RPL and clears ZF");
}

int main(void) {
    TEST_START("JCXZ/ARPL (32-bit)");
    test_jcxz();
    test_arpl();
    TEST_END();
}
