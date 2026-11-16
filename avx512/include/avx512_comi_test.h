#ifndef X86_64_AVX512_COMI_TEST_H
#define X86_64_AVX512_COMI_TEST_H

#include "avx512_test.h"

static uint64_t compare(TEST_VALUE_T left, TEST_VALUE_T right) {
    uint64_t flags;
    __asm__ volatile(
        TEST_MOVE " %1, %%xmm0\n\t"
        TEST_MOVE " %2, %%xmm1\n\t"
        TEST_INSN " %{sae%}, %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(left), "m"(right) : "xmm0", "xmm1", "cc");
    return flags;
}

static void assert_flags(uint64_t flags, int zf, int pf, int cf,
                         const char *label) {
    TEST_ASSERT(!!(flags & ZF_FLAG) == zf, TEST_NAME " %s ZF", label);
    TEST_ASSERT(!!(flags & PF_FLAG) == pf, TEST_NAME " %s PF", label);
    TEST_ASSERT(!!(flags & CF_FLAG) == cf, TEST_NAME " %s CF", label);
    TEST_ASSERT(!(flags & OF_FLAG), TEST_NAME " %s clears OF", label);
    TEST_ASSERT(!(flags & SF_FLAG), TEST_NAME " %s clears SF", label);
    TEST_ASSERT(!(flags & AF_FLAG), TEST_NAME " %s clears AF", label);
}

int main(void) {
    TEST_START(TEST_NAME);
    if (!CHECK_AVX512F()) return 0;

    assert_flags(compare((TEST_VALUE_T)5.0, (TEST_VALUE_T)3.0),
                 0, 0, 0, "greater");
    assert_flags(compare((TEST_VALUE_T)3.0, (TEST_VALUE_T)5.0),
                 0, 0, 1, "less");
    assert_flags(compare((TEST_VALUE_T)4.0, (TEST_VALUE_T)4.0),
                 1, 0, 0, "equal");

    TEST_VALUE_T nan_value;
    TEST_BITS_T nan_bits = TEST_NAN_BITS;
    memcpy(&nan_value, &nan_bits, sizeof(nan_value));

    uint32_t saved_mxcsr, clean_mxcsr, after_mxcsr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved_mxcsr));
    clean_mxcsr = saved_mxcsr & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(clean_mxcsr));
    assert_flags(compare(nan_value, (TEST_VALUE_T)1.0),
                 1, 1, 1, "unordered");
    __asm__ volatile("stmxcsr %0" : "=m"(after_mxcsr));
    __asm__ volatile("ldmxcsr %0" : : "m"(saved_mxcsr));
    TEST_ASSERT((after_mxcsr & UINT32_C(0x3f)) == 0,
                TEST_NAME " SAE suppresses exceptions");
    TEST_END();
}

#endif
