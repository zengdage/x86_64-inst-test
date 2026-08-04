/*
 * Test VMOVDQA64/VMOVDQU64/VMOVAPS/VMOVAPD/VMOVUPS/VMOVUPD/VMOVB/VMOVW/VMOVD/VMOVQ/VMOVSD/VMOVSS
 * with zmm registers (AVX-512F).
 *
 * Compile: gcc -o test_evmov avx512/test_evmov.c -O0 -mavx512f -mavx512bw -mavx512dq -mavx512vl
 * Do NOT use static linking.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
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
    /* Check CPUID leaf 7 for AVX-512F (bit 16 of EBX) */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(7), "c"(0)
    );
    return (ebx >> 16) & 1;
}
#else
#define check_avx512() 1
#endif

int main(void) {
    if (!check_avx512()) {
        printf("AVX-512 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("EVMOV (VMOVDQA64/VMOVDQU64/VMOVAPS/VMOVAPD/VMOVUPS/VMOVUPD/VMOVD/VMOVQ/VMOVSS/VMOVSD)");

    zmm_t src, dst;
    memset(&dst, 0, sizeof(dst));

    /* VMOVDQA64 zmm <- mem */
    for (int i = 0; i < 8; i++) src.u64[i] = (uint64_t)i * 0x0101010101010101ULL;
    __asm__ volatile ("vmovdqa64 %1, %%zmm0\n\t"
                      "vmovdqa64 %%zmm0, %0" : "=m"(dst) : "m"(src) : "zmm0");
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVDQA64 zmm<->mem failed");

    /* VMOVDQU64 zmm <- mem */
    memset(&dst, 0, sizeof(dst));
    for (int i = 0; i < 8; i++) src.u64[i] = 0xFFFFFFFFFFFFFFFFULL - i;
    __asm__ volatile ("vmovdqu64 %1, %%zmm1\n\t"
                      "vmovdqu64 %%zmm1, %0" : "=m"(dst) : "m"(src) : "zmm1");
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVDQU64 zmm<->mem failed");

    /* VMOVAPS zmm */
    memset(&dst, 0, sizeof(dst));
    for (int i = 0; i < 16; i++) src.f32[i] = (float)(i + 1);
    __asm__ volatile ("vmovaps %1, %%zmm2\n\t"
                      "vmovaps %%zmm2, %0" : "=m"(dst) : "m"(src) : "zmm2");
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVAPS zmm<->mem failed");

    /* VMOVAPD zmm */
    memset(&dst, 0, sizeof(dst));
    for (int i = 0; i < 8; i++) src.f64[i] = (double)(i * 3.14);
    __asm__ volatile ("vmovapd %1, %%zmm3\n\t"
                      "vmovapd %%zmm3, %0" : "=m"(dst) : "m"(src) : "zmm3");
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVAPD zmm<->mem failed");

    /* VMOVUPS zmm */
    memset(&dst, 0, sizeof(dst));
    for (int i = 0; i < 16; i++) src.f32[i] = (float)(i * 2 + 1);
    __asm__ volatile ("vmovups %1, %%zmm4\n\t"
                      "vmovups %%zmm4, %0" : "=m"(dst) : "m"(src) : "zmm4");
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVUPS zmm<->mem failed");

    /* VMOVUPD zmm */
    memset(&dst, 0, sizeof(dst));
    for (int i = 0; i < 8; i++) src.f64[i] = (double)(i * 1.5);
    __asm__ volatile ("vmovupd %1, %%zmm5\n\t"
                      "vmovupd %%zmm5, %0" : "=m"(dst) : "m"(src) : "zmm5");
    TEST_ASSERT(memcmp(&src, &dst, 64) == 0, "VMOVUPD zmm<->mem failed");

    /* VMOVDQA64 with merge masking {k1} */
    zmm_t orig, masked;
    for (int i = 0; i < 8; i++) orig.u64[i] = 0xDEADBEEFDEADBEEFULL;
    for (int i = 0; i < 8; i++) src.u64[i] = (uint64_t)(i + 1);
    memcpy(&masked, &orig, 64);
    uint64_t kmask = 0xF0; /* upper 4 lanes */
    __asm__ volatile (
        "kmovq %2, %%k1\n\t"
        "vmovdqa64 %3, %%zmm6\n\t"       /* load orig into zmm6 */
        "vmovdqa64 %4, %%zmm6%{%%k1%}\n\t" /* merge: only lanes where k1=1 updated */
        "vmovdqa64 %%zmm6, %0"
        : "=m"(masked)
        : "m"(orig), "r"(kmask), "m"(orig), "m"(src)
        : "zmm6", "k1"
    );
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (kmask >> i) & 1 ? src.u64[i] : orig.u64[i];
        TEST_ASSERT(masked.u64[i] == expected,
            "VMOVDQA64 merge mask lane %d: got %llx expected %llx",
            i, (unsigned long long)masked.u64[i], (unsigned long long)expected);
    }

    /* VMOVDQA64 with zeroing masking {k1}{z} */
    zmm_t zeroed;
    memset(&zeroed, 0xFF, sizeof(zeroed));
    kmask = 0x0F; /* lower 4 lanes */
    __asm__ volatile (
        "kmovq %2, %%k1\n\t"
        "vmovdqa64 %3, %%zmm7%{%%k1%}%{z%}\n\t"
        "vmovdqa64 %%zmm7, %0"
        : "=m"(zeroed)
        : "m"(zeroed), "r"(kmask), "m"(src)
        : "zmm7", "k1"
    );
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (kmask >> i) & 1 ? src.u64[i] : 0ULL;
        TEST_ASSERT(zeroed.u64[i] == expected,
            "VMOVDQA64 zero mask lane %d: got %llx expected %llx",
            i, (unsigned long long)zeroed.u64[i], (unsigned long long)expected);
    }

    /* VMOVD xmm <- r32 */
    xmm_t xdst;
    memset(&xdst, 0, sizeof(xdst));
    uint32_t val32 = 0xABCD1234;
    __asm__ volatile (
        "vmovd %1, %%xmm0\n\t"
        "vmovdqu %%xmm0, %0"
        : "=m"(xdst) : "r"(val32) : "xmm0"
    );
    TEST_ASSERT(xdst.u32[0] == val32, "VMOVD xmm<-r32 failed: got %x", xdst.u32[0]);
    TEST_ASSERT(xdst.u32[1] == 0 && xdst.u64[1] == 0, "VMOVD zeroes upper 96 bits of xmm");

    /* VMOVQ xmm <- r64 */
    memset(&xdst, 0, sizeof(xdst));
    uint64_t val64 = 0xDEADBEEFCAFEBABEULL;
    __asm__ volatile (
        "vmovq %1, %%xmm1\n\t"
        "vmovdqu %%xmm1, %0"
        : "=m"(xdst) : "r"(val64) : "xmm1"
    );
    TEST_ASSERT(xdst.u64[0] == val64, "VMOVQ xmm<-r64 failed");
    TEST_ASSERT(xdst.u64[1] == 0, "VMOVQ zeroes upper 64 bits of xmm");

    /* VMOVSS xmm */
    memset(&xdst, 0, sizeof(xdst));
    float fval = 3.14159f;
    __asm__ volatile (
        "vmovss %1, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0"
        : "=m"(xdst) : "m"(fval) : "xmm2"
    );
    TEST_ASSERT(xdst.f32[0] == fval, "VMOVSS failed");
    TEST_ASSERT(xdst.u32[1] == 0 && xdst.u64[1] == 0, "VMOVSS memory load zeroes upper xmm bits");

    /* VMOVSD xmm */
    memset(&xdst, 0, sizeof(xdst));
    double dval = 2.718281828;
    __asm__ volatile (
        "vmovsd %1, %%xmm3\n\t"
        "vmovdqu %%xmm3, %0"
        : "=m"(xdst) : "m"(dval) : "xmm3"
    );
    TEST_ASSERT(xdst.f64[0] == dval, "VMOVSD failed");
    TEST_ASSERT(xdst.u64[1] == 0, "VMOVSD memory load zeroes upper 64 bits of xmm");

    /* Masked memory operations access only active lanes, including at a page boundary. */
    long page_size = sysconf(_SC_PAGESIZE);
    uint8_t *pages = mmap(NULL, (size_t)page_size * 2,
                          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(pages != MAP_FAILED, "allocate guard pages for masked VMOVDQU64");
    if (pages != MAP_FAILED) {
        int protected_ok = mprotect(pages + page_size, (size_t)page_size, PROT_NONE) == 0;
        TEST_ASSERT(protected_ok, "protect second VMOVDQU64 guard page");
        if (protected_ok) {
            uint64_t *edge = (uint64_t *)(void *)(pages + page_size - sizeof(uint64_t));
            *edge = UINT64_C(0x0123456789abcdef);
            zmm_t merge_initial, loaded;
            for (int i = 0; i < 8; i++) merge_initial.u64[i] = UINT64_C(0xfeed000000000000) + (uint64_t)i;
            kmask = 1;
            __asm__ volatile (
                "kmovq %3,%%k1\n\tvmovdqu64 %1,%%zmm0\n\t"
                "vmovdqu64 (%2),%%zmm0%{%%k1%}\n\tvmovdqu64 %%zmm0,%0"
                : "=m"(loaded) : "m"(merge_initial), "r"(edge), "r"(kmask)
                : "zmm0", "k1", "memory");
            TEST_ASSERT(loaded.u64[0] == *edge, "masked VMOVDQU64 loads active edge lane");
            for (int i = 1; i < 8; i++)
                TEST_ASSERT(loaded.u64[i] == merge_initial.u64[i],
                            "masked VMOVDQU64 suppresses inactive cross-page lane %d", i);

            __asm__ volatile (
                "kmovq %3,%%k1\n\tvmovdqu64 (%2),%%zmm0%{%%k1%}%{z%}\n\t"
                "vmovdqu64 %%zmm0,%0"
                : "=m"(loaded) : "m"(merge_initial), "r"(edge), "r"(kmask)
                : "zmm0", "k1", "memory");
            TEST_ASSERT(loaded.u64[0] == *edge, "zero-masked VMOVDQU64 active edge lane");
            for (int i = 1; i < 8; i++)
                TEST_ASSERT(loaded.u64[i] == 0,
                            "zero-masked VMOVDQU64 inactive lane %d", i);

            kmask = 0;
            void *invalid = pages + page_size;
            __asm__ volatile (
                "kmovq %2,%%k1\n\tvmovdqu64 (%1),%%zmm0%{%%k1%}%{z%}\n\t"
                "vmovdqu64 %%zmm0,%0"
                : "=m"(loaded) : "r"(invalid), "r"(kmask)
                : "zmm0", "k1", "memory");
            for (int i = 0; i < 8; i++)
                TEST_ASSERT(loaded.u64[i] == 0,
                            "VMOVDQU64 k=0 suppresses PROT_NONE load lane %d", i);

            for (int i = 0; i < 8; i++) src.u64[i] = UINT64_C(0xa500000000000000) + (uint64_t)i;
            kmask = 1;
            __asm__ volatile (
                "kmovq %2,%%k1\n\tvmovdqu64 %1,%%zmm0\n\t"
                "vmovdqu64 %%zmm0,(%0)%{%%k1%}"
                : : "r"(edge), "m"(src), "r"(kmask)
                : "zmm0", "k1", "memory");
            TEST_ASSERT(*edge == src.u64[0],
                        "masked VMOVDQU64 store writes active edge lane without touching guard page");
        }
        munmap(pages, (size_t)page_size * 2);
    }

    TEST_END();
}
