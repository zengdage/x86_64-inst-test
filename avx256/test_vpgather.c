/*
 * Test VPGATHERDD/VPGATHERDQ/VPGATHERQD/VPGATHERQQ
 * AVX2 integer gather instructions (3-operand form with ymm mask)
 * Compile: gcc -o test_vpgather avx256/test_vpgather.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
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

static void test_vpgatherdd(void) {
    TEST_START("VPGATHERDD (gather int32 with int32 indices, 256-bit)");
    int32_t base[16];
    for (int i = 0; i < 16; i++) base[i] = i * 10;
    /* indices: gather elements at indices 3,1,4,1,5,9,2,6 */
    int32_t idx[8] = {3,1,4,1,5,9,2,6};
    int32_t dst[8] = {0,0,0,0,0,0,0,0};
    /* mask: all-ones (high bit set per dword) */
    int32_t mask[8];
    for (int i = 0; i < 8; i++) mask[i] = (int32_t)0x80000000;

    __asm__ volatile(
        "vmovdqu %2, %%ymm1\n\t"   /* indices */
        "vmovdqu %3, %%ymm2\n\t"   /* mask (all-ones) */
        "vmovdqu %4, %%ymm0\n\t"   /* dst (initial) */
        "vpgatherdd %%ymm2, (%5,%%ymm1,4), %%ymm0\n\t"
        "vmovdqu %%ymm0, %0\n\t"
        : "=m"(dst[0])
        : "m"(dst[0]), "m"(idx[0]), "m"(mask[0]), "m"(dst[0]), "r"(base)
        : "ymm0","ymm1","ymm2","memory"
    );
    TEST_ASSERT(dst[0] == 30, "vpgatherdd dst[0]=%d (idx=3)", dst[0]);
    TEST_ASSERT(dst[1] == 10, "vpgatherdd dst[1]=%d (idx=1)", dst[1]);
    TEST_ASSERT(dst[2] == 40, "vpgatherdd dst[2]=%d (idx=4)", dst[2]);
    TEST_ASSERT(dst[4] == 50, "vpgatherdd dst[4]=%d (idx=5)", dst[4]);
    TEST_ASSERT(dst[5] == 90, "vpgatherdd dst[5]=%d (idx=9)", dst[5]);
}

static void test_vpgatherdq(void) {
    TEST_START("VPGATHERDQ (gather int64 with int32 indices, 256-bit)");
    int64_t base[16];
    for (int i = 0; i < 16; i++) base[i] = (int64_t)i * 100;
    /* 4 int32 indices in xmm (lower 128 bits) */
    int32_t idx[4] = {2, 5, 7, 3};
    int64_t dst[4] = {0,0,0,0};
    int64_t mask[4];
    for (int i = 0; i < 4; i++) mask[i] = (int64_t)0x8000000000000000LL;

    __asm__ volatile(
        "vmovdqu %2, %%xmm1\n\t"   /* 4 x int32 indices in xmm */
        "vmovdqu %3, %%ymm2\n\t"   /* mask */
        "vmovdqu %4, %%ymm0\n\t"   /* dst */
        "vpgatherdq %%ymm2, (%5,%%xmm1,8), %%ymm0\n\t"
        "vmovdqu %%ymm0, %0\n\t"
        : "=m"(dst[0])
        : "m"(dst[0]), "m"(idx[0]), "m"(mask[0]), "m"(dst[0]), "r"(base)
        : "ymm0","ymm1","xmm1","ymm2","memory"
    );
    TEST_ASSERT(dst[0] == 200, "vpgatherdq dst[0]=%lld (idx=2)", (long long)dst[0]);
    TEST_ASSERT(dst[1] == 500, "vpgatherdq dst[1]=%lld (idx=5)", (long long)dst[1]);
    TEST_ASSERT(dst[2] == 700, "vpgatherdq dst[2]=%lld (idx=7)", (long long)dst[2]);
    TEST_ASSERT(dst[3] == 300, "vpgatherdq dst[3]=%lld (idx=3)", (long long)dst[3]);
}

static void test_vpgatherqd(void) {
    TEST_START("VPGATHERQD (gather int32 with int64 indices, 128-bit result)");
    int32_t base[16];
    for (int i = 0; i < 16; i++) base[i] = i * 7;
    /* 4 int64 indices in ymm */
    int64_t idx[4] = {1, 3, 5, 7};
    int32_t dst[4] = {0,0,0,0};
    int32_t mask[4];
    for (int i = 0; i < 4; i++) mask[i] = (int32_t)0x80000000;

    __asm__ volatile(
        "vmovdqu %2, %%ymm1\n\t"   /* 4 x int64 indices */
        "vmovdqu %3, %%xmm2\n\t"   /* mask (4 x int32 in xmm) */
        "vmovdqu %4, %%xmm0\n\t"   /* dst */
        "vpgatherqd %%xmm2, (%5,%%ymm1,4), %%xmm0\n\t"
        "vmovdqu %%xmm0, %0\n\t"
        : "=m"(dst[0])
        : "m"(dst[0]), "m"(idx[0]), "m"(mask[0]), "m"(dst[0]), "r"(base)
        : "xmm0","ymm1","xmm2","memory"
    );
    TEST_ASSERT(dst[0] == 7,  "vpgatherqd dst[0]=%d (idx=1)", dst[0]);
    TEST_ASSERT(dst[1] == 21, "vpgatherqd dst[1]=%d (idx=3)", dst[1]);
    TEST_ASSERT(dst[2] == 35, "vpgatherqd dst[2]=%d (idx=5)", dst[2]);
    TEST_ASSERT(dst[3] == 49, "vpgatherqd dst[3]=%d (idx=7)", dst[3]);
}

static void test_vpgatherqq(void) {
    TEST_START("VPGATHERQQ (gather int64 with int64 indices, 256-bit)");
    int64_t base[16];
    for (int i = 0; i < 16; i++) base[i] = (int64_t)i * 1000;
    int64_t idx[4] = {0, 2, 4, 6};
    int64_t dst[4] = {0,0,0,0};
    int64_t mask[4];
    for (int i = 0; i < 4; i++) mask[i] = (int64_t)0x8000000000000000LL;

    __asm__ volatile(
        "vmovdqu %2, %%ymm1\n\t"
        "vmovdqu %3, %%ymm2\n\t"
        "vmovdqu %4, %%ymm0\n\t"
        "vpgatherqq %%ymm2, (%5,%%ymm1,8), %%ymm0\n\t"
        "vmovdqu %%ymm0, %0\n\t"
        : "=m"(dst[0])
        : "m"(dst[0]), "m"(idx[0]), "m"(mask[0]), "m"(dst[0]), "r"(base)
        : "ymm0","ymm1","ymm2","memory"
    );
    TEST_ASSERT(dst[0] == 0,    "vpgatherqq dst[0]=%lld (idx=0)", (long long)dst[0]);
    TEST_ASSERT(dst[1] == 2000, "vpgatherqq dst[1]=%lld (idx=2)", (long long)dst[1]);
    TEST_ASSERT(dst[2] == 4000, "vpgatherqq dst[2]=%lld (idx=4)", (long long)dst[2]);
    TEST_ASSERT(dst[3] == 6000, "vpgatherqq dst[3]=%lld (idx=6)", (long long)dst[3]);
}

static void test_vpgather_boundaries(void) {
    int32_t table[24];
    for (int i = 0; i < 24; i++) table[i] = 1000 + i;
    int32_t idx[8] = {-2, 0, 0, 3, 7, -1, 2, 1};
    int32_t mask[8], mask_after[8], dst[8] = {0};
    for (int i = 0; i < 8; i++) mask[i] = INT32_MIN;
    int32_t *base = &table[8];
    __asm__ volatile(
        "vmovdqu %3,%%ymm1\n\tvmovdqu %4,%%ymm2\n\tvpxor %%ymm0,%%ymm0,%%ymm0\n\t"
        "vpgatherdd %%ymm2,(%2,%%ymm1,4),%%ymm0\n\tvmovdqu %%ymm0,%0\n\tvmovdqu %%ymm2,%1"
        : "=m"(dst), "=m"(mask_after) : "r"(base), "m"(idx), "m"(mask)
        : "ymm0", "ymm1", "ymm2", "memory");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst[i] == base[idx[i]], "vpgatherdd signed/repeated index lane %d", i);
    for (int i = 0; i < 8; i++) TEST_ASSERT(mask_after[i] == 0, "vpgatherdd clears successful mask lane %d", i);

    for (int i = 0; i < 8; i++) idx[i] = i * 4;
    memset(dst, 0, sizeof(dst));
    __asm__ volatile(
        "vmovdqu %2,%%ymm1\n\tvmovdqu %3,%%ymm2\n\tvpxor %%ymm0,%%ymm0,%%ymm0\n\t"
        "vpgatherdd %%ymm2,(%1,%%ymm1,1),%%ymm0\n\tvmovdqu %%ymm0,%0"
        : "=m"(dst) : "r"(table), "m"(idx), "m"(mask) : "ymm0", "ymm1", "ymm2", "memory");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst[i] == table[i], "vpgatherdd scale=1 lane %d", i);

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "vpgather guard page allocation");
    if (guard != MAP_FAILED) {
        int32_t zero_mask[8] = {0};
        int32_t initial[8] = {11,22,33,44,55,66,77,88};
        memset(idx, 0, sizeof(idx));
        __asm__ volatile(
            "vmovdqu %4,%%ymm0\n\tvmovdqu %2,%%ymm1\n\tvmovdqu %3,%%ymm2\n\t"
            "vpgatherdd %%ymm2,(%1,%%ymm1,4),%%ymm0\n\tvmovdqu %%ymm0,%0"
            : "=m"(dst) : "r"(guard), "m"(idx), "m"(zero_mask), "m"(initial)
            : "ymm0", "ymm1", "ymm2", "memory");
        TEST_ASSERT(memcmp(dst, initial, sizeof(dst)) == 0, "vpgatherdd empty mask suppresses invalid-address fault");
        munmap(guard, (size_t)page_size);
    }
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vpgatherdd();
    test_vpgatherdq();
    test_vpgatherqd();
    test_vpgatherqq();
    test_vpgather_boundaries();
    TEST_END();
}
