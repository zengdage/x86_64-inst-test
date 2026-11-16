/* TARGET: VPCOMPRESSD */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

static void check_register(const zmm_test_t *out, const zmm_test_t *src,
                           uint32_t mask, int zeroing) {
    int source = 0;
    for (int lane = 0; lane < 16; lane++) {
        while (source < 16 && !((mask >> source) & 1U)) source++;
        uint32_t expected = source < 16 ? src->u32[source++]
                                       : (zeroing ? 0 : UINT32_C(0xdeadbeef));
        TEST_ASSERT(out->u32[lane] == expected,
                    "VPCOMPRESSD register lane %d", lane);
    }
}

int main(void) {
    zmm_test_t src, initial, out;
    uint32_t memory[16];
    uint32_t mask = UINT32_C(0x8429);

    TEST_START("VPCOMPRESSD");
    if (!CHECK_AVX512F()) return 0;
    for (int i = 0; i < 16; i++) {
        src.u32[i] = UINT32_C(0xabc00000) + (uint32_t)i;
        initial.u32[i] = UINT32_C(0xdeadbeef);
        memory[i] = UINT32_C(0xeeeeeeee);
    }

    __asm__ volatile("kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\t"
                     "vpcompressd %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_register(&out, &src, mask, 0);

    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vpcompressd %%zmm0, %%zmm0%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "k1");
    check_register(&out, &src, mask, 1);

    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vpcompressd %%zmm0, %0%{%%k1%}"
                     : "+m"(memory) : "m"(src), "r"(mask)
                     : "zmm0", "k1", "memory");
    int packed = 0;
    for (int i = 0; i < 16; i++) {
        if ((mask >> i) & 1U)
            TEST_ASSERT(memory[packed++] == src.u32[i],
                        "VPCOMPRESSD memory element %d", packed - 1);
    }
    for (int i = packed; i < 16; i++)
        TEST_ASSERT(memory[i] == UINT32_C(0xeeeeeeee),
                    "VPCOMPRESSD memory footprint element %d", i);

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "VPCOMPRESSD guard page allocation");
    if (guard != MAP_FAILED) {
        uint32_t zero = 0;
        __asm__ volatile("kmovw %1, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
                         "vpcompressd %%zmm0, (%0)%{%%k1%}"
                         : : "r"(guard), "r"(zero), "m"(src)
                         : "zmm0", "k1", "memory");
        TEST_ASSERT(1, "VPCOMPRESSD empty mask suppresses invalid write");
        munmap(guard, (size_t)page_size);
    }
    TEST_END();
}
