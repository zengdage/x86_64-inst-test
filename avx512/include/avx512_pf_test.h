#ifndef X86_64_AVX512_PF_TEST_H
#define X86_64_AVX512_PF_TEST_H

#include "avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

#if TEST_DATA_BITS == 32
typedef uint32_t test_pf_data_t;
#define TEST_SCALE "4"
#define TEST_DATA_INIT(i) (UINT32_C(0x3f800000) + (uint32_t)(i))
#elif TEST_DATA_BITS == 64
typedef uint64_t test_pf_data_t;
#define TEST_SCALE "8"
#define TEST_DATA_INIT(i) (UINT64_C(0x3ff0000000000000) + (uint64_t)(i))
#else
#error "TEST_DATA_BITS must be 32 or 64"
#endif

#if TEST_INDEX_BITS == 32 && TEST_DATA_BITS == 64
#define TEST_INDEX_REG "ymm0"
#define TEST_INDEX_CLOBBER "ymm0"
#define TEST_INDEX_LOAD "vmovdqa32"
#define TEST_KMOV "kmovb"
#define TEST_LANES 8
#define TEST_FULL_MASK UINT32_C(0xff)
#elif TEST_INDEX_BITS == 32
#define TEST_INDEX_REG "zmm0"
#define TEST_INDEX_CLOBBER "zmm0"
#define TEST_INDEX_LOAD "vmovdqa32"
#define TEST_KMOV "kmovw"
#define TEST_LANES 16
#define TEST_FULL_MASK UINT32_C(0xffff)
#else
#define TEST_INDEX_REG "zmm0"
#define TEST_INDEX_CLOBBER "zmm0"
#define TEST_INDEX_LOAD "vmovdqa64"
#define TEST_KMOV "kmovb"
#define TEST_LANES 8
#define TEST_FULL_MASK UINT32_C(0xff)
#endif

int main(void) {
    TEST_START(TEST_NAME);
    if (!CHECK_AVX512PF()) return 0;

    test_pf_data_t data[64], before[64];
    zmm_test_t index = {0};
    for (int i = 0; i < 64; i++) data[i] = TEST_DATA_INIT(i);
    memcpy(before, data, sizeof(data));
    for (int i = 0; i < TEST_LANES; i++) {
#if TEST_INDEX_BITS == 32
        index.i32[i] = i * 3;
#else
        index.i64[i] = i * 3;
#endif
    }

    uint32_t mask = TEST_FULL_MASK;
    __asm__ volatile(
        TEST_KMOV " %2, %%k1\n\t"
        TEST_INDEX_LOAD " %1, %%" TEST_INDEX_REG "\n\t"
        TEST_INSN " (%0,%%" TEST_INDEX_REG "," TEST_SCALE ")%{%%k1%}"
        : : "r"(data), "m"(index), "r"(mask)
        : TEST_INDEX_CLOBBER, "k1", "memory");
    TEST_ASSERT(memcmp(data, before, sizeof(data)) == 0,
                TEST_NAME " does not modify memory");

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, TEST_NAME " guard allocation");
    if (guard != MAP_FAILED) {
        mask = 0;
        __asm__ volatile(
            TEST_KMOV " %2, %%k1\n\t"
            TEST_INDEX_LOAD " %1, %%" TEST_INDEX_REG "\n\t"
            TEST_INSN " (%0,%%" TEST_INDEX_REG "," TEST_SCALE ")%{%%k1%}"
            : : "r"(guard), "m"(index), "r"(mask)
            : TEST_INDEX_CLOBBER, "k1", "memory");
        TEST_ASSERT(memcmp(data, before, sizeof(data)) == 0,
                    TEST_NAME " empty mask suppresses guard page");
        munmap(guard, (size_t)page_size);
    }
    TEST_END();
}

#endif
