/*
 * Test VMOVNTDQ/VMOVNTPD/VMOVNTPS (256-bit non-temporal stores) + VMOVNTDQA (load)
 * Non-temporal (streaming) memory operations bypassing cache
 * Compile: gcc -o test_vmovnt256 avx256/test_vmovnt256.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(7),"c"(0));
    return (ebx >> 5) & 1;
}
#else
#define check_avx2() 1
#endif

static sigjmp_buf movnt_fault_env;
static volatile sig_atomic_t got_movnt_fault;

static void movnt_fault_handler(int sig) {
    (void)sig;
    got_movnt_fault = 1;
    siglongjmp(movnt_fault_env, 1);
}

static void test_vmovnt_alignment_faults(void) {
    unsigned char storage[96] __attribute__((aligned(32))) = {0};
    volatile ymm_t *misaligned = (volatile ymm_t *)(void *)(storage + 4);
    ymm_t src = { .u64 = {1, 2, 3, 4} };
    struct sigaction sa, old_segv, old_bus;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = movnt_fault_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old_segv);
    sigaction(SIGBUS, &sa, &old_bus);

    got_movnt_fault = 0;
    if (sigsetjmp(movnt_fault_env, 1) == 0) {
        __asm__ volatile ("vmovdqu %1, %%ymm0\n\tvmovntdq %%ymm0, %0"
                          : "=m"(*misaligned) : "m"(src) : "ymm0", "memory");
    }
    TEST_ASSERT(got_movnt_fault, "misaligned VMOVNTDQ ymm store raises #GP");

    got_movnt_fault = 0;
    if (sigsetjmp(movnt_fault_env, 1) == 0) {
        ymm_t dst;
        __asm__ volatile ("vmovntdqa %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                          : "=m"(dst) : "m"(*misaligned) : "ymm0", "memory");
    }
    TEST_ASSERT(got_movnt_fault, "misaligned VMOVNTDQA ymm load raises #GP");

    sigaction(SIGSEGV, &old_segv, NULL);
    sigaction(SIGBUS, &old_bus, NULL);
}

static void test_vmovntdq(void) {
    TEST_START("VMOVNTDQ (256-bit non-temporal integer store)");
    /* must be 32-byte aligned */
    int32_t src[8] = {1,2,3,4,5,6,7,8};
    int32_t __attribute__((aligned(32))) dst[8] = {0};
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vmovntdq %%ymm0, %0\n\t"
        "sfence\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "ymm0","memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntdq dst[%d]=%d", i, dst[i]);
}

static void test_vmovntpd(void) {
    TEST_START("VMOVNTPD (256-bit non-temporal double store)");
    double src[4] = {1.1, 2.2, 3.3, 4.4};
    double __attribute__((aligned(32))) dst[4] = {0};
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovntpd %%ymm0, %0\n\t"
        "sfence\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "ymm0","memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntpd dst[%d]=%f", i, dst[i]);
}

static void test_vmovntps(void) {
    TEST_START("VMOVNTPS (256-bit non-temporal float store)");
    float src[8] = {1,2,3,4,5,6,7,8};
    float __attribute__((aligned(32))) dst[8] = {0};
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovntps %%ymm0, %0\n\t"
        "sfence\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "ymm0","memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntps dst[%d]=%f", i, dst[i]);
}

static void test_vmovntdqa(void) {
    TEST_START("VMOVNTDQA (128/256-bit non-temporal integer load)");
    int32_t __attribute__((aligned(32))) src[8] = {
        0, -1, INT32_MIN, INT32_MAX, 10, 20, 30, 40
    };
    int32_t dst[8] = {0};
    __asm__ volatile(
        "vmovntdqa %1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "xmm0","memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntdqa dst[%d]=%d", i, dst[i]);

    memset(dst, 0, sizeof(dst));
    __asm__ volatile(
        "vmovntdqa %1, %%ymm0\n\t"
        "vmovdqu %%ymm0, %0\n\t"
        : "=m"(dst[0]) : "m"(src[0]) : "ymm0","memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == src[i], "vmovntdqa ymm dst[%d]=%d expected %d", i, dst[i], src[i]);
}

static void test_vmovnt_fp_bit_patterns(void) {
    TEST_START("VMOVNTPS/VMOVNTPD exact floating-point bit patterns");
    ymm_t src = { .u32 = {
        UINT32_C(0x00000000), UINT32_C(0x80000000),
        UINT32_C(0x7f800000), UINT32_C(0xff800000),
        UINT32_C(0x7fc12345), UINT32_C(0x7fa54321),
        UINT32_C(0x00000001), UINT32_C(0x7f7fffff)
    } };
    ymm_t dst = {0};

    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovntps %%ymm0, %0\n\t"
        "sfence"
        : "=m"(dst) : "m"(src) : "ymm0", "memory");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u32[i] == src.u32[i],
                    "vmovntps preserves +0/-0, Inf, NaN and subnormal bits lane %d: %#x",
                    i, dst.u32[i]);
    }

    src = (ymm_t){ .u64 = {
        UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000),
        UINT64_C(0x7ff8123456789abc), UINT64_C(0xfff0000000000001)
    } };
    memset(&dst, 0, sizeof(dst));
    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovntpd %%ymm0, %0\n\t"
        "sfence"
        : "=m"(dst) : "m"(src) : "ymm0", "memory");
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.u64[i] == src.u64[i],
                    "vmovntpd preserves signed zero and NaN/Inf payload bits lane %d: %#" PRIx64,
                    i, dst.u64[i]);
    }
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vmovntdq();
    test_vmovntpd();
    test_vmovntps();
    test_vmovntdqa();
    test_vmovnt_fp_bit_patterns();
    test_vmovnt_alignment_faults();
    TEST_END();
}
