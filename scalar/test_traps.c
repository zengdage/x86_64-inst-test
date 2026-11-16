#define _GNU_SOURCE
#include "../common.h"
#include <setjmp.h>
#include <signal.h>

static sigjmp_buf trap_env;
static volatile sig_atomic_t seen_signal;
static volatile sig_atomic_t seen_code;

static void signal_handler(int sig, siginfo_t *info, void *context) {
    (void)context;
    seen_signal = sig;
    seen_code = info->si_code;
    siglongjmp(trap_env, 1);
}

static void run_expected_signal(const char *name, int expected, unsigned encoding) {
    seen_signal = 0;
    seen_code = 0;
    if (sigsetjmp(trap_env, 1) == 0) {
        switch (encoding) {
        case 0: __asm__ volatile ("int3"); break;
        case 1: __asm__ volatile (".byte 0xcd, 0x03"); break;
        case 2: __asm__ volatile (".byte 0xf1"); break;
        case 3: __asm__ volatile (".byte 0xcd, 0x04"); break;
        }
    }
    TEST_ASSERT(seen_signal == expected, "%s raises signal %d (got %d, code %d)",
                name, expected, seen_signal, seen_code);
}

int main(void) {
    struct sigaction action, old_trap, old_segv;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_handler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTRAP, &action, &old_trap);
    sigaction(SIGSEGV, &action, &old_segv);

    TEST_START("INT3/INT imm8/ICEBP traps");
    run_expected_signal("INT3 (CC)", SIGTRAP, 0);
    run_expected_signal("INT 3 (CD 03)", SIGTRAP, 1);
    run_expected_signal("ICEBP", SIGTRAP, 2);
    run_expected_signal("INT 4 privilege check", SIGSEGV, 3);

    sigaction(SIGTRAP, &old_trap, NULL);
    sigaction(SIGSEGV, &old_segv, NULL);
    TEST_END();
}
