#include "../common.h"

static int check_cx16(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1), "c"(0));
    return (ecx >> 13) & 1;
}

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

    TEST_END();
}
