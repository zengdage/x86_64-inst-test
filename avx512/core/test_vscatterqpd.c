/* TARGET: VSCATTERQPD */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    TEST_START("VSCATTERQPD");
    if (!CHECK_AVX512F()) return 0;
    uint64_t output[64];
    zmm_test_t index, values;
    const int64_t indexes[8] = {63, 0, 31, 1, 15, 2, 47, 3};
    for (int i = 0; i < 64; i++) output[i] = UINT64_MAX;
    memcpy(index.i64, indexes, sizeof(indexes));
    for (int i = 0; i < 8; i++) values.u64[i] = UINT64_C(0x4000000000000000) + (uint64_t)i;
    uint32_t mask = UINT32_C(0xa5);
    __asm__ volatile(
        "kmovb %3, %%k1\n\tvmovdqa64 %0, %%zmm0\n\tvmovdqa64 %1, %%zmm1\n\t"
        "vscatterqpd %%zmm0, (%2,%%zmm1,8)%{%%k1%}"
        : : "m"(values), "m"(index), "r"(output), "r"(mask)
        : "zmm0", "zmm1", "k1", "memory");
    for (int pos = 0; pos < 64; pos++) {
        int lane = -1;
        for (int i = 0; i < 8; i++)
            if (((mask >> i) & 1U) && index.i64[i] == pos) lane = i;
        uint64_t expected = lane >= 0 ? values.u64[lane] : UINT64_MAX;
        TEST_ASSERT(output[pos] == expected, "VSCATTERQPD position %d", pos);
    }

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "VSCATTERQPD guard allocation");
    if (guard != MAP_FAILED) {
        mask = 0;
        __asm__ volatile(
            "kmovb %3, %%k1\n\tvmovdqa64 %0, %%zmm0\n\tvmovdqa64 %1, %%zmm1\n\t"
            "vscatterqpd %%zmm0, (%2,%%zmm1,8)%{%%k1%}"
            : : "m"(values), "m"(index), "r"(guard), "r"(mask)
            : "zmm0", "zmm1", "k1", "memory");
        TEST_ASSERT(1, "VSCATTERQPD empty mask suppresses guard page");
        munmap(guard, (size_t)page_size);
    }
    TEST_END();
}
