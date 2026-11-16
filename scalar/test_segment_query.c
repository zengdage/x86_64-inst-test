#define _GNU_SOURCE
#include "../common.h"
#include <setjmp.h>
#include <signal.h>

struct descriptor_register {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct descriptor_store {
    uint8_t before[8];
    struct descriptor_register value;
    uint8_t after[8];
};

static sigjmp_buf query_env;
static volatile sig_atomic_t query_signal;

static void query_signal_handler(int sig) {
    query_signal = sig;
    siglongjmp(query_env, 1);
}

static int run_query(void (*operation)(void *), void *destination) {
    query_signal = 0;
    if (sigsetjmp(query_env, 1) == 0)
        operation(destination);
    return query_signal;
}

static int bytes_are(const void *data, size_t size, uint8_t expected) {
    const uint8_t *bytes = data;
    for (size_t i = 0; i < size; ++i) {
        if (bytes[i] != expected)
            return 0;
    }
    return 1;
}

static void execute_sgdt(void *destination) {
    struct descriptor_register *value = destination;
    __asm__ volatile ("sgdt %0" : "=m"(*value) : : "memory");
}

static void execute_sidt(void *destination) {
    struct descriptor_register *value = destination;
    __asm__ volatile ("sidt %0" : "=m"(*value) : : "memory");
}

static void execute_sldt_register(void *destination) {
    uint16_t value;
    __asm__ volatile ("sldt %0" : "=r"(value));
    *(uint16_t *)destination = value;
}

static void execute_sldt_memory(void *destination) {
    __asm__ volatile ("sldt %0" : "=m"(*(uint16_t *)destination) : : "memory");
}

static void execute_str_register(void *destination) {
    uint16_t value;
    __asm__ volatile ("str %0" : "=r"(value));
    *(uint16_t *)destination = value;
}

static void execute_str_memory(void *destination) {
    __asm__ volatile ("str %0" : "=m"(*(uint16_t *)destination) : : "memory");
}

static void execute_smsw_register(void *destination) {
    uint16_t value;
    __asm__ volatile ("smsw %0" : "=r"(value));
    *(uint16_t *)destination = value;
}

static void execute_smsw_memory(void *destination) {
    __asm__ volatile ("smsw %0" : "=m"(*(uint16_t *)destination) : : "memory");
}

static void test_descriptor_query(const char *name,
                                  void (*operation)(void *)) {
    struct descriptor_store first, second;
    memset(&first, 0xa5, sizeof(first));
    memset(&second, 0x5a, sizeof(second));

    int first_signal = run_query(operation, &first.value);
    int second_signal = run_query(operation, &second.value);
    TEST_ASSERT(first_signal == 0 || first_signal == SIGSEGV,
                "%s executes or raises the UMIP #GP signal (got %d)",
                name, first_signal);
    TEST_ASSERT(second_signal == first_signal,
                "%s has consistent user-mode behavior (%d then %d)",
                name, first_signal, second_signal);

    if (first_signal == 0 && second_signal == 0) {
        TEST_ASSERT(bytes_are(first.before, sizeof(first.before), 0xa5) &&
                    bytes_are(first.after, sizeof(first.after), 0xa5) &&
                    bytes_are(second.before, sizeof(second.before), 0x5a) &&
                    bytes_are(second.after, sizeof(second.after), 0x5a),
                    "%s writes exactly the 10-byte pseudo-descriptor", name);
        TEST_ASSERT(memcmp(&first.value, &second.value,
                           sizeof(first.value)) == 0,
                    "%s returns a stable limit and base", name);
    } else {
        TEST_ASSERT(bytes_are(&first, sizeof(first), 0xa5) &&
                    bytes_are(&second, sizeof(second), 0x5a),
                    "%s fault leaves the destination unchanged", name);
    }
}

static void test_selector_query(const char *name,
                                void (*register_operation)(void *),
                                void (*memory_operation)(void *)) {
    uint16_t register_value = UINT16_C(0xa55a);
    uint16_t memory_value = UINT16_C(0x5aa5);
    int register_signal = run_query(register_operation, &register_value);
    int memory_signal = run_query(memory_operation, &memory_value);

    TEST_ASSERT(register_signal == 0 || register_signal == SIGSEGV,
                "%s register form executes or raises the UMIP #GP signal (got %d)",
                name, register_signal);
    TEST_ASSERT(memory_signal == register_signal,
                "%s register and memory forms have matching behavior (%d/%d)",
                name, register_signal, memory_signal);
    if (register_signal == 0 && memory_signal == 0) {
        TEST_ASSERT(register_value == memory_value,
                    "%s register and memory forms agree: %#x/%#x",
                    name, register_value, memory_value);
    } else {
        TEST_ASSERT(register_value == UINT16_C(0xa55a) &&
                    memory_value == UINT16_C(0x5aa5),
                    "%s fault leaves both destinations unchanged", name);
    }
}

static void test_system_queries(void) {
    test_descriptor_query("SGDT", execute_sgdt);
    test_descriptor_query("SIDT", execute_sidt);
    test_selector_query("SLDT", execute_sldt_register, execute_sldt_memory);
    test_selector_query("STR", execute_str_register, execute_str_memory);

    uint16_t register_value = UINT16_C(0xa55a);
    uint16_t memory_value = UINT16_C(0x5aa5);
    int register_signal = run_query(execute_smsw_register, &register_value);
    int memory_signal = run_query(execute_smsw_memory, &memory_value);
    TEST_ASSERT(register_signal == 0 || register_signal == SIGSEGV,
                "SMSW register form executes or raises the UMIP #GP signal (got %d)",
                register_signal);
    TEST_ASSERT(memory_signal == register_signal,
                "SMSW register and memory forms have matching behavior (%d/%d)",
                register_signal, memory_signal);
    if (register_signal == 0 && memory_signal == 0) {
        TEST_ASSERT(register_value == memory_value,
                    "SMSW register and memory forms agree: %#x/%#x",
                    register_value, memory_value);
        TEST_ASSERT((register_value & UINT16_C(1)) != 0,
                    "SMSW reports CR0.PE set in 64-bit user mode: %#x",
                    register_value);
    } else {
        TEST_ASSERT(register_value == UINT16_C(0xa55a) &&
                    memory_value == UINT16_C(0x5aa5),
                    "SMSW fault leaves both destinations unchanged");
    }
}

static void test_lar_lsl(void) {
    uint16_t cs, ss;
    __asm__ volatile ("movw %%cs, %0\n\tmovw %%ss, %1" : "=r"(cs), "=r"(ss));

    uint32_t access = UINT32_C(0xdeadbeef), limit = UINT32_C(0xdeadbeef);
    uint8_t lar_ok, lsl_ok;
    __asm__ volatile (
        "larl %w[selector], %[access]\n\tsetz %[lar_ok]\n\t"
        "lsll %w[selector], %[limit]\n\tsetz %[lsl_ok]"
        : [access] "+r"(access), [limit] "+r"(limit),
          [lar_ok] "=qm"(lar_ok), [lsl_ok] "=qm"(lsl_ok)
        : [selector] "m"(cs) : "cc");
    TEST_ASSERT(lar_ok && (access & UINT32_C(0x00001f00)) == UINT32_C(0x00001b00),
                "lar accepts user code selector and returns code access rights: %#x", access);
    TEST_ASSERT(lsl_ok && limit == UINT32_MAX,
                "lsl returns flat code segment limit: %#x", limit);

    access = UINT32_C(0x12345678);
    limit = UINT32_C(0x87654321);
    __asm__ volatile (
        "larl %w[selector], %[access]\n\tsetz %[lar_ok]\n\t"
        "lsll %w[selector], %[limit]\n\tsetz %[lsl_ok]"
        : [access] "+r"(access), [limit] "+r"(limit),
          [lar_ok] "=qm"(lar_ok), [lsl_ok] "=qm"(lsl_ok)
        : [selector] "m"((uint16_t){0}) : "cc");
    TEST_ASSERT(!lar_ok && access == UINT32_C(0x12345678),
                "lar null selector fails and preserves destination");
    TEST_ASSERT(!lsl_ok && limit == UINT32_C(0x87654321),
                "lsl null selector fails and preserves destination");
    (void)ss;
}

static void test_verr_verw(void) {
    uint16_t cs, ss, null_selector = 0;
    uint8_t readable, writable, null_readable;
    __asm__ volatile ("movw %%cs, %0\n\tmovw %%ss, %1" : "=r"(cs), "=r"(ss));
    __asm__ volatile ("verr %1\n\tsetz %0" : "=qm"(readable) : "m"(cs) : "cc");
    __asm__ volatile ("verw %1\n\tsetz %0" : "=qm"(writable) : "m"(cs) : "cc");
    TEST_ASSERT(readable && !writable, "code selector is readable but not writable");
    __asm__ volatile ("verw %1\n\tsetz %0" : "=qm"(writable) : "m"(ss) : "cc");
    TEST_ASSERT(writable, "stack data selector is writable");
    __asm__ volatile ("verr %1\n\tsetz %0" : "=qm"(null_readable) : "m"(null_selector) : "cc");
    TEST_ASSERT(!null_readable, "null selector is not readable");
}

int main(void) {
    struct sigaction action, old_segv, old_ill;
    memset(&action, 0, sizeof(action));
    action.sa_handler = query_signal_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, &old_segv);
    sigaction(SIGILL, &action, &old_ill);

    TEST_START("Segment and system register queries");
    test_lar_lsl();
    test_verr_verw();
    test_system_queries();

    sigaction(SIGILL, &old_ill, NULL);
    sigaction(SIGSEGV, &old_segv, NULL);
    TEST_END();
}
