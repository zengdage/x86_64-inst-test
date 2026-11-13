/* AVX-512PF gather/scatter prefetch encodings and empty-mask suppression. */
#include "avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    TEST_START("AVX-512PF gather/scatter prefetch");
    if (!CHECK_AVX512PF()) return 0;
    int32_t data[64], before[64];
    zmm_test_t index;
    for (int i = 0; i < 64; i++) data[i] = i * 17;
    memcpy(before, data, sizeof(data));
    for (int i = 0; i < 16; i++) index.i32[i] = i * 3;
    uint32_t full = 0xffffU;
    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vgatherpf0dps (%0,%%zmm0,4)%{%%k1%}\n\t"
                     "kmovw %2, %%k1\n\tvgatherpf1dps (%0,%%zmm0,4)%{%%k1%}\n\t"
                     "kmovw %2, %%k1\n\tvscatterpf0dps (%0,%%zmm0,4)%{%%k1%}\n\t"
                     "kmovw %2, %%k1\n\tvscatterpf1dps (%0,%%zmm0,4)%{%%k1%}"
                     : : "r"(data), "m"(index), "r"(full)
                     : "zmm0", "k1", "memory");
    TEST_ASSERT(memcmp(data, before, sizeof(data)) == 0, "AVX-512PF never modifies memory");

    long page = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "AVX-512PF guard allocation");
    if (guard != MAP_FAILED) {
        uint32_t zero = 0;
        __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                         "vgatherpf0dps (%0,%%zmm0,4)%{%%k1%}\n\t"
                         "kmovw %2, %%k1\n\tvscatterpf0dps (%0,%%zmm0,4)%{%%k1%}"
                         : : "r"(guard), "m"(index), "r"(zero)
                         : "zmm0", "k1", "memory");
        TEST_ASSERT(1, "empty AVX-512PF masks suppress invalid addresses");
        munmap(guard, (size_t)page);
    }
    TEST_END();
}
