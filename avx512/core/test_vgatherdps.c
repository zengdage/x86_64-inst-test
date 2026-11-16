/* TARGET: VGATHERDPS */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    TEST_START("VGATHERDPS");
    if (!CHECK_AVX512F()) return 0;
    uint32_t table[64];
    zmm_test_t index, initial, out;
    for (int i = 0; i < 64; i++) table[i] = UINT32_C(0x3f800000) + (uint32_t)i;
    const int32_t indexes[16] = {
        63, 0, 31, 1, 15, 2, 47, 3, 7, 48, 8, 32, 16, 4, 62, 5
    };
    memcpy(index.i32, indexes, sizeof(indexes));
    for (int i = 0; i < 16; i++) initial.u32[i] = UINT32_C(0xdead0000) + (uint32_t)i;
    uint32_t mask = UINT32_C(0x8429), mask_out = UINT32_MAX;
    __asm__ volatile(
        "kmovw %5, %%k1\n\tvmovdqa32 %2, %%zmm0\n\tvmovdqa32 %3, %%zmm1\n\t"
        "vgatherdps (%4,%%zmm1,4), %%zmm0%{%%k1%}\n\t"
        "vmovdqa32 %%zmm0, %0\n\tkmovw %%k1, %1"
        : "=m"(out), "=r"(mask_out)
        : "m"(initial), "m"(index), "r"(table), "r"(mask)
        : "zmm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U ? table[index.i32[i]] : initial.u32[i];
        TEST_ASSERT(out.u32[i] == expected, "VGATHERDPS lane %d", i);
    }
    TEST_ASSERT((mask_out & UINT32_C(0xffff)) == 0,
                "VGATHERDPS clears completed mask bits");

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "VGATHERDPS guard allocation");
    if (guard != MAP_FAILED) {
        mask = 0;
        __asm__ volatile(
            "kmovw %4, %%k1\n\tvmovdqa32 %2, %%zmm0\n\tvmovdqa32 %3, %%zmm1\n\t"
            "vgatherdps (%1,%%zmm1,4), %%zmm0%{%%k1%}\n\tvmovdqa32 %%zmm0, %0"
            : "=m"(out) : "r"(guard), "m"(initial), "m"(index), "r"(mask)
            : "zmm0", "zmm1", "k1", "memory");
        TEST_ASSERT(memcmp(&out, &initial, sizeof(out)) == 0,
                    "VGATHERDPS empty mask suppresses guard page");
        munmap(guard, (size_t)page_size);
    }
    TEST_END();
}
