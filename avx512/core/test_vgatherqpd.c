/* TARGET: VGATHERQPD */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    TEST_START("VGATHERQPD");
    if (!CHECK_AVX512F()) return 0;
    uint64_t table[64];
    zmm_test_t index, initial, out;
    for (int i = 0; i < 64; i++) table[i] = UINT64_C(0x3ff0000000000000) + (uint64_t)i;
    const int64_t indexes[8] = {63, 0, 31, 1, 15, 2, 47, 3};
    memcpy(index.i64, indexes, sizeof(indexes));
    for (int i = 0; i < 8; i++) initial.u64[i] = UINT64_C(0xdead000000000000) + (uint64_t)i;
    uint32_t mask = UINT32_C(0xa5), mask_out = UINT32_MAX;
    __asm__ volatile(
        "kmovb %5, %%k1\n\tvmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
        "vgatherqpd (%4,%%zmm1,8), %%zmm0%{%%k1%}\n\t"
        "vmovdqa64 %%zmm0, %0\n\tkmovb %%k1, %1"
        : "=m"(out), "=r"(mask_out)
        : "m"(initial), "m"(index), "r"(table), "r"(mask)
        : "zmm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (mask >> i) & 1U ? table[index.i64[i]] : initial.u64[i];
        TEST_ASSERT(out.u64[i] == expected, "VGATHERQPD lane %d", i);
    }
    TEST_ASSERT((mask_out & UINT32_C(0xff)) == 0,
                "VGATHERQPD clears completed mask bits");

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "VGATHERQPD guard allocation");
    if (guard != MAP_FAILED) {
        mask = 0;
        __asm__ volatile(
            "kmovb %4, %%k1\n\tvmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
            "vgatherqpd (%1,%%zmm1,8), %%zmm0%{%%k1%}\n\tvmovdqa64 %%zmm0, %0"
            : "=m"(out) : "r"(guard), "m"(initial), "m"(index), "r"(mask)
            : "zmm0", "zmm1", "k1", "memory");
        TEST_ASSERT(memcmp(&out, &initial, sizeof(out)) == 0,
                    "VGATHERQPD empty mask suppresses guard page");
        munmap(guard, (size_t)page_size);
    }
    TEST_END();
}
