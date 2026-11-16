#ifndef X86_64_AVX512_FP_LOGIC_TEST_H
#define X86_64_AVX512_FP_LOGIC_TEST_H

#include "avx512_test.h"

#if TEST_BITS == 32
#define TEST_FIELD u32
#define TEST_LANES 16
#define TEST_MASK UINT32_C(0x8001)
#define TEST_MIXED_MASK UINT32_C(0x5a5a)
#define TEST_KMOV "kmovw"
#define TEST_SENTINEL UINT32_C(0xdeadbeef)
typedef uint32_t test_uint_t;
#elif TEST_BITS == 64
#define TEST_FIELD u64
#define TEST_LANES 8
#define TEST_MASK UINT32_C(0x81)
#define TEST_MIXED_MASK UINT32_C(0x5a)
#define TEST_KMOV "kmovb"
#define TEST_SENTINEL UINT64_C(0xdeadbeefcafebabe)
typedef uint64_t test_uint_t;
#else
#error "TEST_BITS must be 32 or 64"
#endif

int main(void) {
    TEST_START(TEST_NAME);
    if (!CHECK_AVX512DQ()) return 0;

    zmm_test_t a, b, initial, out;
    for (int i = 0; i < TEST_LANES; i++) {
        a.TEST_FIELD[i] = (test_uint_t)(TEST_A_PATTERN ^ (test_uint_t)i);
        b.TEST_FIELD[i] = (test_uint_t)(TEST_B_PATTERN ^
                                        ((test_uint_t)i << (TEST_BITS / 2)));
        initial.TEST_FIELD[i] = TEST_SENTINEL;
    }

    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\t"
        "vmovdqa64 %2, %%zmm1\n\t"
        TEST_INSN " %%zmm1, %%zmm0, %%zmm2\n\t"
        "vmovdqa64 %%zmm2, %0"
        : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < TEST_LANES; i++) {
        test_uint_t expected = TEST_EXPECT(a.TEST_FIELD[i], b.TEST_FIELD[i]);
        TEST_ASSERT(out.TEST_FIELD[i] == expected,
                    TEST_NAME " unmasked lane %d", i);
    }

    uint32_t mask = TEST_MASK;
    __asm__ volatile(
        TEST_KMOV " %4, %%k1\n\t"
        "vmovdqa64 %1, %%zmm0\n\t"
        "vmovdqa64 %2, %%zmm1\n\t"
        "vmovdqa64 %3, %%zmm2\n\t"
        TEST_INSN " %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovdqa64 %%zmm2, %0"
        : "=m"(out)
        : "m"(a), "m"(b), "m"(initial), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < TEST_LANES; i++) {
        test_uint_t expected = (mask >> i) & 1U
            ? TEST_EXPECT(a.TEST_FIELD[i], b.TEST_FIELD[i])
            : TEST_SENTINEL;
        TEST_ASSERT(out.TEST_FIELD[i] == expected,
                    TEST_NAME " merge mask lane %d", i);
    }

    mask = TEST_MIXED_MASK;
    __asm__ volatile(
        TEST_KMOV " %3, %%k1\n\t"
        "vmovdqa64 %1, %%zmm0\n\t"
        "vmovdqa64 %2, %%zmm1\n\t"
        TEST_INSN " %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqa64 %%zmm2, %0"
        : "=m"(out) : "m"(a), "m"(b), "r"(mask)
        : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < TEST_LANES; i++) {
        test_uint_t expected = (mask >> i) & 1U
            ? TEST_EXPECT(a.TEST_FIELD[i], b.TEST_FIELD[i]) : 0;
        TEST_ASSERT(out.TEST_FIELD[i] == expected,
                    TEST_NAME " zero mask lane %d", i);
    }
    TEST_END();
}

#endif
