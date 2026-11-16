#define _GNU_SOURCE
#include "../common.h"
#include <setjmp.h>
#include <signal.h>

#if !defined(__i386__)
#error "Build this test with -m32"
#endif

static sigjmp_buf fault_env;
static volatile sig_atomic_t seen_signal;

static void fault_handler(int sig) {
    seen_signal = sig;
    siglongjmp(fault_env, 1);
}

static void test_bound(void) {
    struct { int32_t lower, upper; } bounds = {-10, 20};
    int32_t value = -10;
    seen_signal = 0;
    __asm__ volatile ("boundl %0, %1" : "+r"(value) : "m"(bounds));
    TEST_ASSERT(!seen_signal && value == -10, "bound accepts inclusive lower limit");
    value = 20;
    __asm__ volatile ("boundl %0, %1" : "+r"(value) : "m"(bounds));
    TEST_ASSERT(value == 20, "bound accepts inclusive upper limit");
    value = 21;
    seen_signal = 0;
    if (sigsetjmp(fault_env, 1) == 0)
        __asm__ volatile ("boundl %0, %1" : "+r"(value) : "m"(bounds));
    TEST_ASSERT(seen_signal == SIGSEGV, "bound out of range raises SIGSEGV/#BR");
}

static void test_into(void) {
    seen_signal = 0;
    if (sigsetjmp(fault_env, 1) == 0)
        __asm__ volatile ("movb $0x7f, %%al\n\taddb $1, %%al\n\tinto" : : : "eax", "cc");
    TEST_ASSERT(seen_signal == SIGSEGV, "into with OF=1 raises SIGSEGV/#OF");
    seen_signal = 0;
    __asm__ volatile ("xorl %%eax, %%eax\n\tinto" : : : "eax", "cc");
    TEST_ASSERT(!seen_signal, "into with OF=0 falls through");
}

int main(void) {
    struct sigaction action, old_segv;
    memset(&action, 0, sizeof(action));
    action.sa_handler = fault_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, &old_segv);
    TEST_START("BOUND/INTO (32-bit)");
    test_bound();
    test_into();
    sigaction(SIGSEGV, &old_segv, NULL);
    TEST_END();
}
