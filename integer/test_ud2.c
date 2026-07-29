/*
 * test_ud2.c - Test x86-64 UD2 instruction
 *
 * UD2 generates an invalid opcode exception (#UD). It is used to:
 *   - Deliberately crash/trap for debugging
 *   - Mark unreachable code
 *
 * We cannot execute UD2 directly (it would crash). Instead we:
 *   - Verify UD2 can be assembled (encoding test)
 *   - Use signal handling to catch the SIGILL it generates
 *
 * Compile: gcc -o test_ud2 integer/test_ud2.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <signal.h>
#include <setjmp.h>

static sigjmp_buf jmpbuf;
static volatile int got_sigill = 0;

static void sigill_handler(int sig) {
    (void)sig;
    got_sigill = 1;
    siglongjmp(jmpbuf, 1);
}

static void test_ud2_encoding(void) {
    /* Verify UD2 is 2 bytes: 0x0F 0x0B */
    unsigned char ud2_bytes[2];
    __asm__ volatile (
        "jmp 1f\n\t"
        "0: ud2\n\t"
        "1:\n\t"
        "leaq 0b(%%rip), %%rax\n\t"
        "movb (%%rax), %%cl\n\t"
        "movb %%cl, %0\n\t"
        "movb 1(%%rax), %%cl\n\t"
        "movb %%cl, %1"
        : "=r"(ud2_bytes[0]), "=r"(ud2_bytes[1])
        :
        : "rax", "rcx"
    );
    TEST_ASSERT(ud2_bytes[0] == 0x0F && ud2_bytes[1] == 0x0B,
                "ud2 encoding: expected 0F 0B, got %02x %02x",
                ud2_bytes[0], ud2_bytes[1]);
}

static void test_ud2_generates_sigill(void) {
    struct sigaction sa, old_sa;
    sa.sa_handler = sigill_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGILL, &sa, &old_sa);

    got_sigill = 0;
    if (sigsetjmp(jmpbuf, 1) == 0) {
        __asm__ volatile ("ud2");
        /* Should not reach here */
        TEST_ASSERT(0, "ud2: should have generated SIGILL");
    } else {
        TEST_ASSERT(got_sigill == 1, "ud2: SIGILL caught successfully");
    }

    sigaction(SIGILL, &old_sa, NULL);
}

int main(void) {
    TEST_START("UD2 instruction");
    test_ud2_encoding();
    test_ud2_generates_sigill();
    TEST_END();
}
