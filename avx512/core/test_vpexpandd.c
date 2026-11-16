/* TARGET: VPEXPANDD */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

static void check_result(const zmm_test_t *out, const uint32_t *packed,
                         uint32_t mask, int zeroing) {
    int input = 0;
    for (int lane = 0; lane < 16; lane++) {
        uint32_t expected = (mask >> lane) & 1U
            ? packed[input++] : (zeroing ? 0 : UINT32_C(0xcccccccc));
        TEST_ASSERT(out->u32[lane] == expected,
                    "VPEXPANDD lane %d", lane);
    }
}

int main(void) {
    zmm_test_t src, initial, out;
    uint32_t packed[16];
    uint32_t mask = UINT32_C(0x8429);

    TEST_START("VPEXPANDD");
    if (!CHECK_AVX512F()) return 0;
    for (int i = 0; i < 16; i++) {
        src.u32[i] = UINT32_C(0xabc00000) + (uint32_t)i;
        packed[i] = UINT32_C(0x12340000) + (uint32_t)i;
        initial.u32[i] = UINT32_C(0xcccccccc);
    }

    __asm__ volatile("kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\t"
                     "vpexpandd %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, src.u32, mask, 0);

    __asm__ volatile("kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vpexpandd %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    check_result(&out, src.u32, mask, 1);

    __asm__ volatile("kmovw %3, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
                     "vpexpandd %1, %%zmm0%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(out)
                     : "m"(packed), "m"(initial), "r"(mask)
                     : "zmm0", "k1", "memory");
    check_result(&out, packed, mask, 0);

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "VPEXPANDD guard page allocation");
    if (guard != MAP_FAILED) {
        uint32_t zero = 0;
        __asm__ volatile("kmovw %1, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
                         "vpexpandd (%0), %%zmm0%{%%k1%}"
                         : : "r"(guard), "r"(zero), "m"(initial)
                         : "zmm0", "k1", "memory");
        TEST_ASSERT(1, "VPEXPANDD empty mask suppresses invalid read");
        munmap(guard, (size_t)page_size);
    }
    TEST_END();
}
