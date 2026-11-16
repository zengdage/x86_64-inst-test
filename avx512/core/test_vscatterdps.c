/* TARGET: VSCATTERDPS */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    TEST_START("VSCATTERDPS");
    if (!CHECK_AVX512F()) return 0;
    uint32_t output[64];
    zmm_test_t index, values;
    const int32_t indexes[16] = {
        63, 0, 31, 1, 15, 2, 47, 3, 7, 48, 8, 32, 16, 4, 62, 5
    };
    for (int i = 0; i < 64; i++) output[i] = UINT32_MAX;
    memcpy(index.i32, indexes, sizeof(indexes));
    for (int i = 0; i < 16; i++) values.u32[i] = UINT32_C(0x40000000) + (uint32_t)i;
    uint32_t mask = UINT32_C(0x8429);
    __asm__ volatile(
        "kmovw %3, %%k1\n\tvmovdqa32 %0, %%zmm0\n\tvmovdqa32 %1, %%zmm1\n\t"
        "vscatterdps %%zmm0, (%2,%%zmm1,4)%{%%k1%}"
        : : "m"(values), "m"(index), "r"(output), "r"(mask)
        : "zmm0", "zmm1", "k1", "memory");
    for (int pos = 0; pos < 64; pos++) {
        int lane = -1;
        for (int i = 0; i < 16; i++)
            if (((mask >> i) & 1U) && index.i32[i] == pos) lane = i;
        uint32_t expected = lane >= 0 ? values.u32[lane] : UINT32_MAX;
        TEST_ASSERT(output[pos] == expected, "VSCATTERDPS position %d", pos);
    }

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "VSCATTERDPS guard allocation");
    if (guard != MAP_FAILED) {
        mask = 0;
        __asm__ volatile(
            "kmovw %3, %%k1\n\tvmovdqa32 %0, %%zmm0\n\tvmovdqa32 %1, %%zmm1\n\t"
            "vscatterdps %%zmm0, (%2,%%zmm1,4)%{%%k1%}"
            : : "m"(values), "m"(index), "r"(guard), "r"(mask)
            : "zmm0", "zmm1", "k1", "memory");
        TEST_ASSERT(1, "VSCATTERDPS empty mask suppresses guard page");
        munmap(guard, (size_t)page_size);
    }
    TEST_END();
}
