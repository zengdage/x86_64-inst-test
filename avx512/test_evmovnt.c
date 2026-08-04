/*
 * Test VMOVNTDQ/VMOVNTDQA/VMOVNTPD/VMOVNTPS with zmm registers (AVX-512F).
 * Non-temporal (streaming) store/load instructions.
 *
 * Compile: gcc -o test_evmovnt avx512/test_evmovnt.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "../common.h"

typedef union {
    long long    i64[8];
    int          i32[16];
    short        i16[32];
    signed char  i8[64];
    unsigned long long u64[8];
    unsigned int       u32[16];
    unsigned short     u16[32];
    unsigned char      u8[64];
    float  f32[16];
    double f64[8];
} zmm_t __attribute__((aligned(64)));

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 16) & 1;
}
#else
#define check_avx512() 1
#endif

static sigjmp_buf movnt_fault_env;
static volatile sig_atomic_t got_movnt_fault;

static void movnt_fault_handler(int sig) {
    (void)sig;
    got_movnt_fault = 1;
    siglongjmp(movnt_fault_env, 1);
}

static void test_zmm_alignment_faults(void) {
    unsigned char storage[160] __attribute__((aligned(64))) = {0};
    volatile zmm_t *misaligned = (volatile zmm_t *)(void *)(storage + 4);
    zmm_t src = { .u64 = {1,2,3,4,5,6,7,8} };
    struct sigaction sa, old_segv, old_bus;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = movnt_fault_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old_segv);
    sigaction(SIGBUS, &sa, &old_bus);

    got_movnt_fault = 0;
    if (sigsetjmp(movnt_fault_env, 1) == 0)
        __asm__ volatile ("vmovdqu64 %1, %%zmm0\n\tvmovntdq %%zmm0, %0"
                          : "=m"(*misaligned) : "m"(src) : "zmm0", "memory");
    TEST_ASSERT(got_movnt_fault, "misaligned VMOVNTDQ zmm store raises #GP");

    got_movnt_fault = 0;
    if (sigsetjmp(movnt_fault_env, 1) == 0)
        __asm__ volatile ("vmovntdqa %0, %%zmm0" : : "m"(*misaligned) : "zmm0", "memory");
    TEST_ASSERT(got_movnt_fault, "misaligned VMOVNTDQA zmm load raises #GP");

    sigaction(SIGSEGV, &old_segv, NULL);
    sigaction(SIGBUS, &old_bus, NULL);
}

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVMOVNT (VMOVNTDQ/VMOVNTDQA/VMOVNTPD/VMOVNTPS)");

    zmm_t src, dst;

    /* VMOVNTDQ: non-temporal store zmm -> mem */
    for (int i = 0; i < 8; i++) src.u64[i] = 0xA5A5A5A5A5A5A5A5ULL ^ (uint64_t)i;
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovdqa64 %1, %%zmm0\n\t"
        "vmovntdq %%zmm0, %0\n\t" "sfence"
        : "=m"(dst) : "m"(src) : "zmm0", "memory"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQ store failed");

    /* VMOVNTDQA: non-temporal load mem -> zmm */
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovntdqa %1, %%zmm1\n\t"
        "vmovdqa64 %%zmm1, %0"
        : "=m"(dst) : "m"(src) : "zmm1"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQA load failed");

    /* VMOVNTPD: non-temporal store packed doubles */
    for (int i = 0; i < 8; i++) src.f64[i] = (double)(i + 1) * 1.5;
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovapd %1, %%zmm2\n\t"
        "vmovntpd %%zmm2, %0\n\t" "sfence"
        : "=m"(dst) : "m"(src) : "zmm2", "memory"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTPD store failed");

    /* VMOVNTPS: non-temporal store packed floats */
    for (int i = 0; i < 16; i++) src.f32[i] = (float)(i + 1) * 0.5f;
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovaps %1, %%zmm3\n\t"
        "vmovntps %%zmm3, %0\n\t" "sfence"
        : "=m"(dst) : "m"(src) : "zmm3", "memory"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTPS store failed");

    /* Boundary: all zeros */
    memset(&src, 0, sizeof(src));
    memset(&dst, 0xFF, sizeof(dst));
    __asm__ volatile (
        "vmovdqa64 %1, %%zmm4\n\t"
        "vmovntdq %%zmm4, %0\n\t" "sfence"
        : "=m"(dst) : "m"(src) : "zmm4", "memory"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQ all-zeros failed");

    /* Boundary: all ones */
    memset(&src, 0xFF, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovdqa64 %1, %%zmm5\n\t"
        "vmovntdq %%zmm5, %0\n\t" "sfence"
        : "=m"(dst) : "m"(src) : "zmm5", "memory"
    );
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVNTDQ all-ones failed");

    test_zmm_alignment_faults();

    TEST_END();
}
