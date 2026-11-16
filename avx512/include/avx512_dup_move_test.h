#ifndef X86_64_AVX512_DUP_MOVE_TEST_H
#define X86_64_AVX512_DUP_MOVE_TEST_H

#include "avx512_test.h"

#if TEST_BITS == 32
#define TEST_FIELD u32
#define TEST_LANES 16
#define TEST_KMOV "kmovw"
#define TEST_ENDPOINT_MASK UINT32_C(0x8001)
#define TEST_MIXED_MASK UINT32_C(0x5a5a)
#define TEST_SENTINEL UINT32_C(0xdeadbeef)
typedef uint32_t test_dup_uint_t;
#elif TEST_BITS == 64
#define TEST_FIELD u64
#define TEST_LANES 8
#define TEST_KMOV "kmovb"
#define TEST_ENDPOINT_MASK UINT32_C(0x81)
#define TEST_MIXED_MASK UINT32_C(0x5a)
#define TEST_SENTINEL UINT64_C(0xdeadbeefcafebabe)
typedef uint64_t test_dup_uint_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

int main(void) {
    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;
    zmm_test_t src, initial, out;
    for (int i = 0; i < TEST_LANES; i++) {
        src.TEST_FIELD[i] = (test_dup_uint_t)(TEST_BASE + (test_dup_uint_t)i);
        initial.TEST_FIELD[i] = TEST_SENTINEL;
    }

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\t"
                     TEST_INSN " %%zmm0, %%zmm1\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    for (int i = 0; i < TEST_LANES; i++)
        TEST_ASSERT(out.TEST_FIELD[i] == src.TEST_FIELD[TEST_SOURCE_INDEX(i)],
                    TEST_NAME " unmasked lane %d", i);

    uint32_t mask = TEST_ENDPOINT_MASK;
    __asm__ volatile(TEST_KMOV " %3, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     "vmovdqa64 %2, %%zmm1\n\t"
                     TEST_INSN " %%zmm0, %%zmm1%{%%k1%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out)
                     : "m"(src), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < TEST_LANES; i++) {
        test_dup_uint_t expected = (mask >> i) & 1U
            ? src.TEST_FIELD[TEST_SOURCE_INDEX(i)] : TEST_SENTINEL;
        TEST_ASSERT(out.TEST_FIELD[i] == expected,
                    TEST_NAME " merge mask lane %d", i);
    }

    mask = TEST_MIXED_MASK;
    __asm__ volatile(TEST_KMOV " %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
                     TEST_INSN " %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
                     "vmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(src), "r"(mask)
                     : "zmm0", "zmm1", "k1");
    for (int i = 0; i < TEST_LANES; i++) {
        test_dup_uint_t expected = (mask >> i) & 1U
            ? src.TEST_FIELD[TEST_SOURCE_INDEX(i)] : 0;
        TEST_ASSERT(out.TEST_FIELD[i] == expected,
                    TEST_NAME " zero mask lane %d", i);
    }
    TEST_END();
}

#endif
