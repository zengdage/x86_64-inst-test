/*
 * Test VMASKMOVPD/VMASKMOVPS/VPMASKMOVD/VPMASKMOVQ
 * Conditional (masked) load/store: high bit of each mask element controls operation
 * Compile: gcc -o test_vmaskmov avx256/test_vmaskmov.c -O0 -mavx2
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

static void test_vmaskmovpd_load(void) {
    TEST_START("VMASKMOVPD load (256-bit)");
    double src[4] = {1.0, 2.0, 3.0, 4.0};
    double dst[4] = {0.0, 0.0, 0.0, 0.0};
    /* mask: high bit of each 64-bit lane; set lanes 0 and 2 */
    int64_t mask[4] = {(int64_t)0x8000000000000000LL, 0,
                       (int64_t)0x8000000000000000LL, 0};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"   /* mask */
        "vmaskmovpd %1, %%ymm0, %%ymm1\n\t"
        "vmovupd %%ymm1, %0\n\t"
        : "=m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == 1.0, "vmaskmovpd load dst[0]=%f", dst[0]);
    TEST_ASSERT(dst[1] == 0.0, "vmaskmovpd load dst[1]=%f (masked out)", dst[1]);
    TEST_ASSERT(dst[2] == 3.0, "vmaskmovpd load dst[2]=%f", dst[2]);
    TEST_ASSERT(dst[3] == 0.0, "vmaskmovpd load dst[3]=%f (masked out)", dst[3]);
}

static void test_vmaskmovpd_store(void) {
    TEST_START("VMASKMOVPD store (256-bit)");
    double src[4] = {10.0, 20.0, 30.0, 40.0};
    double dst[4] = {-1.0, -2.0, -3.0, -4.0};
    int64_t mask[4] = {(int64_t)0x8000000000000000LL, 0,
                       0, (int64_t)0x8000000000000000LL};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"
        "vmovupd %1, %%ymm1\n\t"
        "vmaskmovpd %%ymm1, %%ymm0, %0\n\t"
        : "+m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == 10.0, "vmaskmovpd store dst[0]=%f", dst[0]);
    TEST_ASSERT(dst[1] == -2.0, "vmaskmovpd store dst[1]=%f (not written)", dst[1]);
    TEST_ASSERT(dst[2] == -3.0, "vmaskmovpd store dst[2]=%f (not written)", dst[2]);
    TEST_ASSERT(dst[3] == 40.0, "vmaskmovpd store dst[3]=%f", dst[3]);
}

static void test_vmaskmovps_load(void) {
    TEST_START("VMASKMOVPS load (256-bit)");
    float src[8] = {1,2,3,4,5,6,7,8};
    float dst[8] = {0,0,0,0,0,0,0,0};
    /* mask: high bit of each 32-bit lane; set even lanes */
    int32_t mask[8] = {(int32_t)0x80000000, 0, (int32_t)0x80000000, 0,
                       (int32_t)0x80000000, 0, (int32_t)0x80000000, 0};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"
        "vmaskmovps %1, %%ymm0, %%ymm1\n\t"
        "vmovups %%ymm1, %0\n\t"
        : "=m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == 1.0f, "vmaskmovps load dst[0]=%f", dst[0]);
    TEST_ASSERT(dst[1] == 0.0f, "vmaskmovps load dst[1]=%f (masked)", dst[1]);
    TEST_ASSERT(dst[2] == 3.0f, "vmaskmovps load dst[2]=%f", dst[2]);
    TEST_ASSERT(dst[4] == 5.0f, "vmaskmovps load dst[4]=%f", dst[4]);
}

static void test_vmaskmovps_store(void) {
    TEST_START("VMASKMOVPS store (256-bit)");
    float src[8] = {10,20,30,40,50,60,70,80};
    float dst[8] = {-1,-2,-3,-4,-5,-6,-7,-8};
    int32_t mask[8] = {0, (int32_t)0x80000000, 0, (int32_t)0x80000000,
                       0, (int32_t)0x80000000, 0, (int32_t)0x80000000};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"
        "vmovups %1, %%ymm1\n\t"
        "vmaskmovps %%ymm1, %%ymm0, %0\n\t"
        : "+m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == -1.0f, "vmaskmovps store dst[0]=%f (not written)", dst[0]);
    TEST_ASSERT(dst[1] == 20.0f, "vmaskmovps store dst[1]=%f", dst[1]);
    TEST_ASSERT(dst[3] == 40.0f, "vmaskmovps store dst[3]=%f", dst[3]);
}

static void test_vpmaskmovd_load(void) {
    TEST_START("VPMASKMOVD load (256-bit, AVX2)");
    int32_t src[8] = {1,2,3,4,5,6,7,8};
    int32_t dst[8] = {0,0,0,0,0,0,0,0};
    int32_t mask[8] = {(int32_t)0x80000000,(int32_t)0x80000000,0,0,
                       (int32_t)0x80000000,(int32_t)0x80000000,0,0};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"
        "vpmaskmovd %1, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == 1, "vpmaskmovd load dst[0]=%d", dst[0]);
    TEST_ASSERT(dst[1] == 2, "vpmaskmovd load dst[1]=%d", dst[1]);
    TEST_ASSERT(dst[2] == 0, "vpmaskmovd load dst[2]=%d (masked)", dst[2]);
    TEST_ASSERT(dst[4] == 5, "vpmaskmovd load dst[4]=%d", dst[4]);
}

static void test_vpmaskmovd_store(void) {
    TEST_START("VPMASKMOVD store (256-bit, AVX2)");
    int32_t src[8] = {10,20,30,40,50,60,70,80};
    int32_t dst[8] = {-1,-2,-3,-4,-5,-6,-7,-8};
    int32_t mask[8] = {0,0,(int32_t)0x80000000,(int32_t)0x80000000,
                       0,0,(int32_t)0x80000000,(int32_t)0x80000000};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"
        "vmovdqu %1, %%ymm1\n\t"
        "vpmaskmovd %%ymm1, %%ymm0, %0\n\t"
        : "+m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == -1, "vpmaskmovd store dst[0]=%d (not written)", dst[0]);
    TEST_ASSERT(dst[2] == 30, "vpmaskmovd store dst[2]=%d", dst[2]);
    TEST_ASSERT(dst[3] == 40, "vpmaskmovd store dst[3]=%d", dst[3]);
}

static void test_vpmaskmovq_load(void) {
    TEST_START("VPMASKMOVQ load (256-bit, AVX2)");
    int64_t src[4] = {100, 200, 300, 400};
    int64_t dst[4] = {0, 0, 0, 0};
    int64_t mask[4] = {(int64_t)0x8000000000000000LL, 0,
                       0, (int64_t)0x8000000000000000LL};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"
        "vpmaskmovq %1, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == 100, "vpmaskmovq load dst[0]=%lld", (long long)dst[0]);
    TEST_ASSERT(dst[1] == 0,   "vpmaskmovq load dst[1]=%lld (masked)", (long long)dst[1]);
    TEST_ASSERT(dst[3] == 400, "vpmaskmovq load dst[3]=%lld", (long long)dst[3]);
}

static void test_vpmaskmovq_store(void) {
    TEST_START("VPMASKMOVQ store (256-bit, AVX2)");
    int64_t src[4] = {1000, 2000, 3000, 4000};
    int64_t dst[4] = {-1, -2, -3, -4};
    int64_t mask[4] = {0, (int64_t)0x8000000000000000LL,
                       (int64_t)0x8000000000000000LL, 0};
    __asm__ volatile(
        "vmovdqu %2, %%ymm0\n\t"
        "vmovdqu %1, %%ymm1\n\t"
        "vpmaskmovq %%ymm1, %%ymm0, %0\n\t"
        : "+m"(dst[0]) : "m"(src[0]), "m"(mask[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(dst[0] == -1,   "vpmaskmovq store dst[0]=%lld (not written)", (long long)dst[0]);
    TEST_ASSERT(dst[1] == 2000, "vpmaskmovq store dst[1]=%lld", (long long)dst[1]);
    TEST_ASSERT(dst[2] == 3000, "vpmaskmovq store dst[2]=%lld", (long long)dst[2]);
    TEST_ASSERT(dst[3] == -4,   "vpmaskmovq store dst[3]=%lld (not written)", (long long)dst[3]);
}

static void test_maskmov_boundaries(void) {
    float src[8] = {1,2,3,4,5,6,7,8}, dst[8];
    uint32_t sign_masks[8] = {0x7fffffffu,0x80000000u,1,0xffffffffu,0,0x80000001u,0x40000000u,0x80000000u};
    __asm__ volatile("vmovdqu %2,%%ymm0\n\tvmaskmovps %1,%%ymm0,%%ymm1\n\tvmovups %%ymm1,%0"
        : "=m"(dst) : "m"(src), "m"(sign_masks) : "ymm0", "ymm1");
    for (int i = 0; i < 8; i++) TEST_ASSERT(dst[i] == ((sign_masks[i] & 0x80000000u) ? src[i] : 0.0f), "vmaskmovps uses only sign bit lane %d", i);

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "vmaskmov guard page allocation");
    if (guard != MAP_FAILED) {
        uint32_t zero_mask[8] = {0};
        __asm__ volatile("vmovdqu %2,%%ymm0\n\tvmaskmovps %1,%%ymm0,%%ymm1\n\tvmovups %%ymm1,%0"
            : "=m"(dst) : "m"(*(const char (*)[32])guard), "m"(zero_mask) : "ymm0", "ymm1");
        for (int i = 0; i < 8; i++) TEST_ASSERT(dst[i] == 0.0f, "vmaskmovps zero mask invalid page lane %d", i);

        int32_t store_src[8] = {1,2,3,4,5,6,7,8};
        __asm__ volatile("vmovdqu %1,%%ymm0\n\tvmovdqu %2,%%ymm1\n\tvpmaskmovd %%ymm0,%%ymm1,%0"
            : "=m"(*(char (*)[32])guard) : "m"(store_src), "m"(zero_mask) : "ymm0", "ymm1", "memory");
        TEST_ASSERT(1, "vpmaskmovd zero-mask store suppresses invalid-address fault");
        munmap(guard, (size_t)page_size);
    }
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vmaskmovpd_load();
    test_vmaskmovpd_store();
    test_vmaskmovps_load();
    test_vmaskmovps_store();
    test_vpmaskmovd_load();
    test_vpmaskmovd_store();
    test_vpmaskmovq_load();
    test_vpmaskmovq_store();
    test_maskmov_boundaries();
    TEST_END();
}
