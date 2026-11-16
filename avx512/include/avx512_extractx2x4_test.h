#ifndef X86_64_AVX512_EXTRACTX2X4_TEST_H
#define X86_64_AVX512_EXTRACTX2X4_TEST_H

#include "avx512_test.h"

#if TEST_BITS == 32
#define TEST_FIELD u32
#define TEST_LANES 4
#define TEST_MASK UINT32_C(0x9)
typedef uint32_t test_uint_t;
#elif TEST_BITS == 64
#define TEST_FIELD u64
#define TEST_LANES 2
#define TEST_MASK UINT32_C(0x1)
typedef uint64_t test_uint_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

static void check_chunk(const xmm_t *out, const zmm_test_t *src, int chunk,
                        uint32_t mask, int zeroing) {
    for (int i = 0; i < TEST_LANES; i++) {
        test_uint_t expected = (mask >> i) & 1U
            ? src->TEST_FIELD[chunk * TEST_LANES + i]
            : (zeroing ? 0 : (test_uint_t)TEST_SENTINEL);
        TEST_ASSERT(out->TEST_FIELD[i] == expected,
                    TEST_NAME " chunk %d element %d", chunk, i);
    }
}

int main(void) {
    zmm_test_t src;
    xmm_t chunks[4], wrapped, merged, zeroed;
    uint32_t mask = TEST_MASK;

    TEST_START(TEST_NAME);
    if (!CHECK_FEATURE()) return 0;
    for (int i = 0; i < 512 / TEST_BITS; i++)
        src.TEST_FIELD[i] = (test_uint_t)(TEST_BASE + (test_uint_t)i);
    for (int i = 0; i < TEST_LANES; i++)
        merged.TEST_FIELD[i] = (test_uint_t)TEST_SENTINEL;

    __asm__ volatile("vmovdqa64 %4, %%zmm0\n\t"
                     TEST_INSN " $0, %%zmm0, %0\n\t"
                     TEST_INSN " $1, %%zmm0, %1\n\t"
                     TEST_INSN " $2, %%zmm0, %2\n\t"
                     TEST_INSN " $3, %%zmm0, %3"
                     : "=m"(chunks[0]), "=m"(chunks[1]),
                       "=m"(chunks[2]), "=m"(chunks[3])
                     : "m"(src) : "zmm0");
    for (int chunk = 0; chunk < 4; chunk++)
        check_chunk(&chunks[chunk], &src, chunk, (1U << TEST_LANES) - 1U, 0);

    __asm__ volatile("vmovdqa64 %2, %%ymm0\n\t"
                     TEST_INSN " $0, %%ymm0, %0\n\t"
                     TEST_INSN " $1, %%ymm0, %1"
                     : "=m"(chunks[0]), "=m"(chunks[1])
                     : "m"(src) : "ymm0");
    check_chunk(&chunks[0], &src, 0, (1U << TEST_LANES) - 1U, 0);
    check_chunk(&chunks[1], &src, 1, (1U << TEST_LANES) - 1U, 0);

    __asm__ volatile("vmovdqa64 %3, %%zmm0\n\t"
                     TEST_INSN " $0xff, %%zmm0, %0\n\t"
                     "kmovw %4, %%k1\n\tvmovdqa64 %1, %%xmm1\n\t"
                     TEST_INSN " $3, %%zmm0, %%xmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%xmm1, %1\n\t"
                     TEST_INSN " $3, %%zmm0, %%xmm2%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%xmm2, %2"
                     : "=m"(wrapped), "+m"(merged), "=m"(zeroed)
                     : "m"(src), "r"(mask)
                     : "zmm0", "xmm1", "xmm2", "k1");
    check_chunk(&wrapped, &src, 3, (1U << TEST_LANES) - 1U, 0);
    check_chunk(&merged, &src, 3, mask, 0);
    check_chunk(&zeroed, &src, 3, mask, 1);
    TEST_END();
}

#endif
