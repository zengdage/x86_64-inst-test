/*
 * test_prefetch.c - Test x86-64 PREFETCH instructions
 *
 * PREFETCHT0:  Prefetch data to all cache levels.
 * PREFETCHT1:  Prefetch data to L2 and above.
 * PREFETCHT2:  Prefetch data to L3 and above.
 * PREFETCHNTA: Prefetch data with non-temporal hint.
 * PREFETCHW:   Prefetch data for writing (3DNow! / PREFETCHW extension).
 * These are hints; they do not affect correctness or flags.
 * We just verify they execute without faulting.
 *
 * Compile: gcc -o test_prefetch integer/test_prefetch.c -O0 -mprfchw
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <unistd.h>

static sigjmp_buf prefetch_fault_env;
static volatile sig_atomic_t got_prefetch_fault;

static void prefetch_fault_handler(int sig) {
    (void)sig;
    got_prefetch_fault = 1;
    siglongjmp(prefetch_fault_env, 1);
}

static void test_prefetcht0(void) {
    volatile uint64_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    __asm__ volatile ("prefetcht0 %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 1, "prefetcht0: data accessible");
}

static void test_prefetcht1(void) {
    volatile uint64_t data[8] = {10, 20, 30, 40, 50, 60, 70, 80};

    __asm__ volatile ("prefetcht1 %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 10, "prefetcht1: data accessible");
}

static void test_prefetcht2(void) {
    volatile uint64_t data[8] = {100, 200, 300, 400, 500, 600, 700, 800};

    __asm__ volatile ("prefetcht2 %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 100, "prefetcht2: data accessible");
}

static void test_prefetchnta(void) {
    volatile uint64_t data[8] = {0};

    __asm__ volatile ("prefetchnta %0" : : "m"(data[0]));
    TEST_ASSERT(data[0] == 0, "prefetchnta: data accessible");
}

static void test_prefetch_no_flags(void) {
    volatile uint64_t data = 42;
    uint64_t flags_before, flags_after;

    __asm__ volatile (
        "stc\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "prefetcht0 %2\n\t"
        "pushfq\n\t"
        "popq %1"
        : "=r"(flags_before), "=r"(flags_after)
        : "m"(data)
    );
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "prefetch: flags unchanged");
}

static void test_prefetch_array(void) {
    volatile uint64_t arr[128];
    for (int i = 0; i < 128; i++) arr[i] = i;

    /* Prefetch multiple cache lines */
    for (int i = 0; i < 128; i += 8) {
        __asm__ volatile ("prefetcht0 %0" : : "m"(arr[i]));
    }

    /* Verify data still correct */
    int correct = 1;
    for (int i = 0; i < 128; i++) {
        if (arr[i] != (uint64_t)i) { correct = 0; break; }
    }
    TEST_ASSERT(correct, "prefetch array: data intact");
}

static void test_prefetch_fault_suppression(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    void *page = mmap(NULL, (size_t)page_size, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(page != MAP_FAILED, "allocate PROT_NONE prefetch guard page");
    if (page == MAP_FAILED) return;

    struct sigaction sa, old_segv, old_bus;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = prefetch_fault_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old_segv);
    sigaction(SIGBUS, &sa, &old_bus);
    got_prefetch_fault = 0;
    if (sigsetjmp(prefetch_fault_env, 1) == 0) {
        volatile char *p = page;
        __asm__ volatile (
            "prefetcht0 %0\n\t" "prefetcht1 %0\n\t" "prefetcht2 %0\n\t"
            "prefetchnta %0"
            : : "m"(*p));
    }
    sigaction(SIGSEGV, &old_segv, NULL);
    sigaction(SIGBUS, &old_bus, NULL);
    TEST_ASSERT(!got_prefetch_fault, "prefetch hints suppress PROT_NONE page faults");
    munmap(page, (size_t)page_size);
}

int main(void) {
    TEST_START("PREFETCH instructions");
    test_prefetcht0();
    test_prefetcht1();
    test_prefetcht2();
    test_prefetchnta();
    test_prefetch_no_flags();
    test_prefetch_array();
    test_prefetch_fault_suppression();
    TEST_END();
}
