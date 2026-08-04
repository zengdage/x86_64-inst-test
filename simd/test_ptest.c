/*
 * test_ptest.c - Test PTEST instruction (SSE4.1)
 *
 * PTEST: Performs AND and ANDNOT of two 128-bit values and sets ZF and CF.
 *   ZF = 1 if (src1 AND src2) == 0
 *   CF = 1 if (NOT(src1) AND src2) == 0  (i.e., all set bits in src2 are also in src1)
 *
 * Compile: gcc -o test_ptest simd/test_ptest.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_ptest_all_zero(void) {
    xmm_t a = { .u64 = { 0, 0 } };
    xmm_t b = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    uint64_t flags;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "ptest %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "xmm1", "cc"
    );
    /* a AND b = 0 => ZF=1 */
    TEST_ASSERT(flags & ZF_FLAG, "ptest 0 AND all_ones: ZF should be set");
    /* NOT(a) AND b = all_ones AND all_ones = all_ones != 0 => CF=0 */
    TEST_ASSERT(!(flags & CF_FLAG), "ptest 0 AND all_ones: CF should be clear");
}

static void test_ptest_all_ones(void) {
    xmm_t a = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    xmm_t b = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    uint64_t flags;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "ptest %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "xmm1", "cc"
    );
    /* a AND b = all_ones != 0 => ZF=0 */
    TEST_ASSERT(!(flags & ZF_FLAG), "ptest all_ones: ZF should be clear");
    /* NOT(a) AND b = 0 AND all_ones = 0 => CF=1 */
    TEST_ASSERT(flags & CF_FLAG, "ptest all_ones: CF should be set (all bits of b in a)");
}

static void test_ptest_partial(void) {
    xmm_t a = { .u64 = { 0xFF00FF00FF00FF00ULL, 0 } };
    xmm_t b = { .u64 = { 0xFF00FF00FF00FF00ULL, 0 } };
    uint64_t flags;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "ptest %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "xmm1", "cc"
    );
    /* a AND b != 0 => ZF=0 */
    TEST_ASSERT(!(flags & ZF_FLAG), "ptest partial: ZF clear");
    /* NOT(a) AND b = 0 (b's bits are subset of a's) => CF=1 */
    TEST_ASSERT(flags & CF_FLAG, "ptest partial: CF set (b subset of a)");
}

static void test_ptest_disjoint(void) {
    xmm_t a = { .u64 = { 0xFF00FF0000000000ULL, 0 } };
    xmm_t b = { .u64 = { 0x00FF00FF00000000ULL, 0 } };
    uint64_t flags;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "ptest %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "xmm1", "cc"
    );
    /* a AND b = 0 => ZF=1 */
    TEST_ASSERT(flags & ZF_FLAG, "ptest disjoint: ZF set");
    /* NOT(a) AND b = b != 0 => CF=0 */
    TEST_ASSERT(!(flags & CF_FLAG), "ptest disjoint: CF clear");
}

static void test_ptest_both_zero(void) {
    xmm_t a = { .u64 = { 0, 0 } };
    xmm_t b = { .u64 = { 0, 0 } };
    uint64_t flags;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movdqa %2, %%xmm1\n\t"
        "ptest %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ptest both zero: ZF set");
    TEST_ASSERT(flags & CF_FLAG, "ptest both zero: CF set");
}

static void test_ptest_clears_other_flags(void) {
    xmm_t a = { .u64 = {UINT64_C(0x8000000000000001), 0} };
    xmm_t b = { .u64 = {UINT64_C(0x8000000000000001), 0} };
    uint64_t flags;
    __asm__ volatile (
        "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "movdqa %1, %%xmm0\n\t" "ptest %2, %%xmm0\n\t"
        "pushfq\n\t" "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "r11", "cc");
    TEST_ASSERT(!(flags & ZF_FLAG) && (flags & CF_FLAG),
                "ptest reference case sets ZF=0,CF=1");
    TEST_ASSERT((flags & (OF_FLAG | SF_FLAG | AF_FLAG | PF_FLAG)) == 0,
                "ptest clears OF/SF/AF/PF");
}

int main(void) {
    TEST_START("PTEST instruction");
    test_ptest_all_zero();
    test_ptest_all_ones();
    test_ptest_partial();
    test_ptest_disjoint();
    test_ptest_both_zero();
    test_ptest_clears_other_flags();
    TEST_END();
}
