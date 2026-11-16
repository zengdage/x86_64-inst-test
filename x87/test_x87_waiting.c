#include "../common.h"
#include <setjmp.h>
#include <signal.h>

typedef struct {
    uint8_t bytes[10];
} ext80_t;

static sigjmp_buf fpe_env;
static volatile sig_atomic_t seen_fpe;
static volatile sig_atomic_t seen_fpe_code;
static uint16_t original_cw;

static void fpe_handler(int sig, siginfo_t *info, void *context) {
    (void)context;
    seen_fpe = sig;
    seen_fpe_code = info->si_code;
    __asm__ volatile ("fnclex");
    siglongjmp(fpe_env, 1);
}

static void prepare_pending_divide_by_zero(void) {
    const double zero = 0.0;
    uint16_t unmasked_cw = UINT16_C(0x037f) & ~UINT16_C(1u << 2);
    __asm__ volatile (
        "fninit\n\tfld1\n\tfdivl %[zero]\n\tfstp %%st(0)\n\t"
        "fldcw %[unmasked_cw]"
        : : [zero] "m"(zero), [unmasked_cw] "m"(unmasked_cw) : "memory");
}

static void restore_x87(void) {
    __asm__ volatile ("fnclex\n\tfldcw %0" : : "m"(original_cw));
}

static void test_fnop(void) {
    const ext80_t value = {{0x35, 0xc2, 0x68, 0x21, 0xa2, 0xda, 0x0f, 0xc9, 0x00, 0x40}};
    ext80_t result;
    uint16_t before, after;
    __asm__ volatile (
        "fninit\n\tfldt %[value]\n\tfnstsw %[before]\n\t"
        "fnop\n\tfnstsw %[after]\n\tfstpt %[result]"
        : [before] "=m"(before), [after] "=m"(after), [result] "=m"(result)
        : [value] "m"(value));
    TEST_ASSERT(memcmp(&value, &result, sizeof(value)) == 0, "fnop preserves ST(0)");
    TEST_ASSERT(before == after, "fnop preserves x87 status word");
}

static void test_finit(void) {
    uint16_t altered_cw = UINT16_C(0x077f), cw, sw;
    const double value = 1.0;
    __asm__ volatile (
        "fldcw %[altered]\n\tfldl %[value]\n\tfinit\n\t"
        "fnstcw %[cw]\n\tfnstsw %[sw]"
        : [cw] "=m"(cw), [sw] "=m"(sw)
        : [altered] "m"(altered_cw), [value] "m"(value));
    TEST_ASSERT(cw == UINT16_C(0x037f), "finit restores default control word: %#x", cw);
    TEST_ASSERT(sw == 0, "finit clears status and stack state: %#x", sw);
}

static void test_fstcw_fstenv(void) {
    uint16_t configured = UINT16_C(0x0b7f), stored_cw = 0;
    uint8_t environment[28] __attribute__((aligned(4)));
    memset(environment, 0, sizeof(environment));
    __asm__ volatile (
        "fninit\n\tfldcw %[configured]\n\t"
        "fstcw %[stored]\n\tfstenv %[environment]\n\t"
        "fldenv %[environment]"
        : [stored] "=m"(stored_cw), [environment] "+m"(environment)
        : [configured] "m"(configured));
    uint16_t env_cw;
    memcpy(&env_cw, environment, sizeof(env_cw));
    TEST_ASSERT(stored_cw == configured, "fstcw stores configured control word");
    TEST_ASSERT(env_cw == configured, "fstenv stores configured control word");

    uint16_t stored_sw;
    __asm__ volatile (
        "fninit\n\tfld1\n\tfld1\n\tfcompp\n\tfstsw %0"
        : "=m"(stored_sw));
    TEST_ASSERT((stored_sw & UINT16_C(0x4500)) == UINT16_C(0x4000),
                "waiting fstsw m16 stores x87 comparison status");
    __asm__ volatile ("fninit");
}

static void test_fsave_frstor(void) {
    uint8_t state[108] __attribute__((aligned(16)));
    const ext80_t value = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xfe, 0x3f}};
    ext80_t result;
    uint16_t after_save_cw;
    memset(state, 0, sizeof(state));
    __asm__ volatile (
        "fninit\n\tfldt %[value]\n\tfsave %[state]\n\t"
        "fnstcw %[after_save_cw]\n\tfrstor %[state]\n\tfstpt %[result]"
        : [state] "+m"(state), [after_save_cw] "=m"(after_save_cw),
          [result] "=m"(result)
        : [value] "m"(value));
    TEST_ASSERT(after_save_cw == UINT16_C(0x037f), "fsave reinitializes x87 state");
    TEST_ASSERT(memcmp(&value, &result, sizeof(value)) == 0,
                "fsave/frstor preserves exact 80-bit value");
}

static void test_waiting_encodings(void) {
    const unsigned char *finit_p, *fstcw_p, *fstsw_p, *fstenv_p, *fsave_p;
    __asm__ volatile (
        "jmp 1f\n\t0: finit\n\t1: leaq 0b(%%rip), %[finit]\n\t"
        "jmp 3f\n\t2: fstcw (%%rsp)\n\t3: leaq 2b(%%rip), %[fstcw]\n\t"
        "jmp 5f\n\t4: fstsw (%%rsp)\n\t5: leaq 4b(%%rip), %[fstsw]\n\t"
        "jmp 7f\n\t6: fstenv (%%rsp)\n\t7: leaq 6b(%%rip), %[fstenv]\n\t"
        "jmp 9f\n\t8: fsave (%%rsp)\n\t9: leaq 8b(%%rip), %[fsave]"
        : [finit] "=r"(finit_p), [fstcw] "=r"(fstcw_p),
          [fstsw] "=r"(fstsw_p), [fstenv] "=r"(fstenv_p),
          [fsave] "=r"(fsave_p));
    TEST_ASSERT(finit_p[0] == 0x9b, "finit includes WAIT prefix");
    TEST_ASSERT(fstcw_p[0] == 0x9b, "fstcw includes WAIT prefix");
    TEST_ASSERT(fstsw_p[0] == 0x9b, "fstsw m16 includes WAIT prefix");
    TEST_ASSERT(fstenv_p[0] == 0x9b, "fstenv includes WAIT prefix");
    TEST_ASSERT(fsave_p[0] == 0x9b, "fsave includes WAIT prefix");
}

enum x87_state_operation {
    OP_INIT,
    OP_STCW,
    OP_STSW,
    OP_STENV,
    OP_SAVE,
    OP_WAIT
};

static const char *operation_name(enum x87_state_operation operation) {
    static const char *const names[] = {
        "init", "stcw", "stsw", "stenv", "save", "wait"
    };
    return names[operation];
}

static void execute_state_operation(enum x87_state_operation operation, int waiting) {
    uint8_t state[108] __attribute__((aligned(16)));
    uint8_t environment[28] __attribute__((aligned(4)));
    uint16_t word = 0;
    switch (operation) {
    case OP_INIT:
        if (waiting) __asm__ volatile ("finit");
        else __asm__ volatile ("fninit");
        break;
    case OP_STCW:
        if (waiting) __asm__ volatile ("fstcw %0" : "=m"(word));
        else __asm__ volatile ("fnstcw %0" : "=m"(word));
        break;
    case OP_STSW:
        if (waiting) __asm__ volatile ("fstsw %0" : "=m"(word));
        else __asm__ volatile ("fnstsw %0" : "=m"(word));
        break;
    case OP_STENV:
        if (waiting) __asm__ volatile ("fstenv %0" : "=m"(environment));
        else __asm__ volatile ("fnstenv %0" : "=m"(environment));
        break;
    case OP_SAVE:
        if (waiting) __asm__ volatile ("fsave %0" : "=m"(state));
        else __asm__ volatile ("fnsave %0" : "=m"(state));
        break;
    case OP_WAIT:
        __asm__ volatile ("fwait");
        break;
    }
}

static void test_pending_exception_behavior(void) {
    for (enum x87_state_operation operation = OP_INIT; operation <= OP_WAIT; ++operation) {
        seen_fpe = 0;
        seen_fpe_code = 0;
        if (sigsetjmp(fpe_env, 1) == 0) {
            prepare_pending_divide_by_zero();
            execute_state_operation(operation, 1);
        }
        restore_x87();
        TEST_ASSERT(seen_fpe == SIGFPE,
                    "waiting f%s raises SIGFPE for a pending unmasked exception (got %d, code %d)",
                    operation_name(operation), seen_fpe, seen_fpe_code);
    }

    for (enum x87_state_operation operation = OP_INIT; operation < OP_WAIT; ++operation) {
        seen_fpe = 0;
        seen_fpe_code = 0;
        if (sigsetjmp(fpe_env, 1) == 0) {
            prepare_pending_divide_by_zero();
            execute_state_operation(operation, 0);
        }
        restore_x87();
        TEST_ASSERT(seen_fpe == 0,
                    "non-waiting fn%s does not raise for a pending exception",
                    operation_name(operation));
    }
}

int main(void) {
    struct sigaction action, old_fpe;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = fpe_handler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGFPE, &action, &old_fpe);
    __asm__ volatile ("fnstcw %0" : "=m"(original_cw));

    TEST_START("x87 FNOP and waiting state instructions");
    test_fnop();
    test_finit();
    test_fstcw_fstenv();
    test_fsave_frstor();
    test_waiting_encodings();
    test_pending_exception_behavior();
    restore_x87();
    sigaction(SIGFPE, &old_fpe, NULL);
    TEST_END();
}
