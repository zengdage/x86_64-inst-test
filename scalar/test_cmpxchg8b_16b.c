#include "../common.h"
#include <signal.h>
#include <setjmp.h>

static sigjmp_buf cmpxchg_fault_env;
static volatile sig_atomic_t got_cmpxchg_fault;

static void cmpxchg_fault_handler(int sig) {
    (void)sig;
    got_cmpxchg_fault = 1;
    siglongjmp(cmpxchg_fault_env, 1);
}

static void test_cmpxchg16b_misaligned(void) {
    unsigned char storage[32] __attribute__((aligned(16))) = {0};
    volatile __int128 *misaligned = (volatile __int128 *)(void *)(storage + 8);
    uint64_t rax = 0, rdx = 0, rbx = 1, rcx = 2;
    struct sigaction sa, old_segv, old_bus;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cmpxchg_fault_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old_segv);
    sigaction(SIGBUS, &sa, &old_bus);

    got_cmpxchg_fault = 0;
    if (sigsetjmp(cmpxchg_fault_env, 1) == 0) {
        __asm__ volatile ("lock cmpxchg16b %0"
                          : "+m"(*misaligned), "+a"(rax), "+d"(rdx)
                          : "b"(rbx), "c"(rcx) : "cc");
    }
    sigaction(SIGSEGV, &old_segv, NULL);
    sigaction(SIGBUS, &old_bus, NULL);
    TEST_ASSERT(got_cmpxchg_fault, "misaligned CMPXCHG16B raises #GP");
}

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_cx16(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1), "c"(0));
    return (ecx >> 13) & 1;
}
#else
#define check_cx16() 1
#endif

int main(void) {
    TEST_START("CMPXCHG8B/CMPXCHG16B");

    /* CMPXCHG8B: equal case - should swap */
    {
        uint64_t mem = 0x0000000200000001ULL; /* EDX:EAX = 2:1 */
        uint32_t eax_in = 1, edx_in = 2;
        uint32_t ecx_in = 0xDEAD, ebx_in = 0xBEEF;
        uint8_t zf = 0;
        __asm__ volatile(
            "movl %3, %%eax\n\t"
            "movl %4, %%edx\n\t"
            "movl %5, %%ecx\n\t"
            "movl %6, %%ebx\n\t"
            "cmpxchg8b %1\n\t"
            "setz %0"
            : "=r"(zf), "+m"(mem)
            : "m"(mem), "r"(eax_in), "r"(edx_in), "r"(ecx_in), "r"(ebx_in)
            : "eax", "ebx", "ecx", "edx");
        uint32_t lo = (uint32_t)(mem & 0xFFFFFFFF);
        uint32_t hi = (uint32_t)(mem >> 32);
        TEST_ASSERT(zf == 1 && lo == 0xBEEF && hi == 0xDEAD,
                    "cmpxchg8b equal: zf=%u lo=0x%x hi=0x%x", zf, lo, hi);
    }

    /* CMPXCHG8B: not-equal case - should load EDX:EAX from mem */
    {
        uint64_t mem = 0x0000000400000003ULL; /* mem = 4:3 */
        uint32_t eax_out = 0, edx_out = 0;
        uint8_t zf = 0;
        __asm__ volatile(
            "movl $1, %%eax\n\t"   /* eax != mem_lo */
            "movl $2, %%edx\n\t"
            "movl $0xDEAD, %%ecx\n\t"
            "movl $0xBEEF, %%ebx\n\t"
            "cmpxchg8b %3\n\t"
            "setz %0\n\t"
            "movl %%eax, %1\n\t"
            "movl %%edx, %2"
            : "=r"(zf), "=r"(eax_out), "=r"(edx_out), "+m"(mem)
            :
            : "eax", "ebx", "ecx", "edx");
        TEST_ASSERT(zf == 0 && eax_out == 3 && edx_out == 4,
                    "cmpxchg8b neq: zf=%u eax=%u edx=%u", zf, eax_out, edx_out);
        /* mem should be unchanged */
        TEST_ASSERT(mem == 0x0000000400000003ULL, "cmpxchg8b neq: mem unchanged");
    }

    /* CMPXCHG8B: low half matches but high half mismatches. */
    {
        uint64_t mem = UINT64_C(0x0000000400000001);
        uint32_t eax = 1, edx = 2;
        uint8_t zf;
        __asm__ volatile (
            "cmpxchg8b %1\n\t" "setz %0"
            : "=q"(zf), "+m"(mem), "+a"(eax), "+d"(edx)
            : "b"(UINT32_C(0xffffffff)), "c"(UINT32_C(0xffffffff)) : "cc");
        TEST_ASSERT(zf == 0 && eax == 1 && edx == 4,
                    "cmpxchg8b high-half mismatch loads complete memory value");
        TEST_ASSERT(mem == UINT64_C(0x0000000400000001),
                    "cmpxchg8b high-half mismatch leaves memory unchanged");
    }

    /* CMPXCHG8B: all-ones boundary replaced with all zeros. */
    {
        uint64_t mem = UINT64_MAX;
        uint32_t eax = UINT32_MAX, edx = UINT32_MAX;
        uint8_t zf;
        __asm__ volatile (
            "cmpxchg8b %1\n\t" "setz %0"
            : "=q"(zf), "+m"(mem), "+a"(eax), "+d"(edx)
            : "b"(0U), "c"(0U) : "cc");
        TEST_ASSERT(zf == 1 && mem == 0, "cmpxchg8b all-ones to zero boundary");
    }

    if (!check_cx16()) {
        printf("CMPXCHG16B not supported, skipping\n");
        TEST_END();
    }

    /* CMPXCHG16B: equal case */
    {
        __int128 mem __attribute__((aligned(16)));
        uint64_t *p = (uint64_t *)&mem;
        p[0] = 0x1111111111111111ULL;
        p[1] = 0x2222222222222222ULL;
        uint64_t rax = p[0], rdx = p[1];
        uint64_t rbx = 0xAAAAAAAAAAAAAAAAULL;
        uint64_t rcx = 0xBBBBBBBBBBBBBBBBULL;
        uint8_t zf = 0;
        __asm__ volatile(
            "lock cmpxchg16b %1\n\t"
            "setz %0"
            : "=r"(zf), "+m"(mem), "+a"(rax), "+d"(rdx)
            : "b"(rbx), "c"(rcx));
        TEST_ASSERT(zf == 1 && p[0] == rbx && p[1] == rcx,
                    "cmpxchg16b equal: zf=%u", zf);
    }

    /* CMPXCHG16B: not-equal case */
    {
        __int128 mem __attribute__((aligned(16)));
        uint64_t *p = (uint64_t *)&mem;
        p[0] = 0x3333333333333333ULL;
        p[1] = 0x4444444444444444ULL;
        uint64_t rax = 0x1111111111111111ULL; /* mismatch */
        uint64_t rdx = 0x2222222222222222ULL;
        uint64_t rbx = 0xAAAAAAAAAAAAAAAAULL;
        uint64_t rcx = 0xBBBBBBBBBBBBBBBBULL;
        uint8_t zf = 0;
        __asm__ volatile(
            "lock cmpxchg16b %1\n\t"
            "setz %0"
            : "=r"(zf), "+m"(mem), "+a"(rax), "+d"(rdx)
            : "b"(rbx), "c"(rcx));
        TEST_ASSERT(zf == 0 && rax == p[0] && rdx == p[1],
                    "cmpxchg16b neq: zf=%u rax=0x%lx rdx=0x%lx", zf, rax, rdx);
        TEST_ASSERT(p[0] == 0x3333333333333333ULL, "cmpxchg16b neq: mem unchanged");
    }

    /* CMPXCHG16B: low qword matches but high qword mismatches. */
    {
        __int128 mem __attribute__((aligned(16)));
        uint64_t *p = (uint64_t *)&mem;
        p[0] = UINT64_C(0x1111111111111111);
        p[1] = UINT64_C(0x4444444444444444);
        uint64_t rax = p[0], rdx = UINT64_C(0x2222222222222222);
        uint8_t zf;
        __asm__ volatile (
            "lock cmpxchg16b %1\n\t" "setz %0"
            : "=q"(zf), "+m"(mem), "+a"(rax), "+d"(rdx)
            : "b"(UINT64_MAX), "c"(UINT64_MAX) : "cc");
        TEST_ASSERT(zf == 0 && rax == p[0] && rdx == p[1],
                    "cmpxchg16b high-half mismatch loads complete memory value");
        TEST_ASSERT(p[0] == UINT64_C(0x1111111111111111) &&
                    p[1] == UINT64_C(0x4444444444444444),
                    "cmpxchg16b high-half mismatch leaves memory unchanged");
    }

    /* CMPXCHG16B: all-zero boundary replaced with all ones. */
    {
        __int128 mem __attribute__((aligned(16))) = 0;
        uint64_t *p = (uint64_t *)&mem;
        uint64_t rax = 0, rdx = 0;
        uint8_t zf;
        __asm__ volatile (
            "lock cmpxchg16b %1\n\t" "setz %0"
            : "=q"(zf), "+m"(mem), "+a"(rax), "+d"(rdx)
            : "b"(UINT64_MAX), "c"(UINT64_MAX) : "cc");
        TEST_ASSERT(zf == 1 && p[0] == UINT64_MAX && p[1] == UINT64_MAX,
                    "cmpxchg16b zero to all-ones boundary");
    }

    test_cmpxchg16b_misaligned();

    TEST_END();
}
