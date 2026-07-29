/*
 * test_vtestps_pd.c - Test VTESTPS/VTESTPD instructions (AVX)
 *
 * VTESTPS: Test packed single-precision sign bits against another register.
 *          Sets ZF if (src1 AND src2) sign bits all zero.
 *          Sets CF if (NOT(src1) AND src2) sign bits all zero.
 * VTESTPD: Same for packed double-precision.
 *
 * Compile: gcc -o test_vtestps_pd simd/test_vtestps_pd.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vtestps_all_positive(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    uint64_t flags;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vtestps %2, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "cc"
    );
    /* Both positive => all sign bits 0 => AND=0 => ZF=1 */
    TEST_ASSERT(flags & ZF_FLAG, "vtestps all positive: ZF set");
    /* NOT(a) sign bits = 1 for all, AND with b sign bits = 0 => CF=1 */
    TEST_ASSERT(flags & CF_FLAG, "vtestps all positive: CF set");
}

static void test_vtestps_mixed(void) {
    xmm_t a = { .f32 = {-1.0f, 2.0f, -3.0f, 4.0f} };
    xmm_t b = { .f32 = {-1.0f, -2.0f, 3.0f, 4.0f} };
    uint64_t flags;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vtestps %2, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "cc"
    );
    /* a AND b sign bits: a[0] neg, b[0] neg => 1; others => 0. Not all zero => ZF=0 */
    TEST_ASSERT(!(flags & ZF_FLAG), "vtestps mixed: ZF clear");
}

static void test_vtestpd_basic(void) {
    xmm_t a = { .f64 = {-1.0, -2.0} };
    xmm_t b = { .f64 = {-3.0, -4.0} };
    uint64_t flags;

    __asm__ volatile (
        "vmovapd %1, %%xmm0\n\t"
        "vtestpd %2, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0", "cc"
    );
    /* All negative: AND of sign bits = all 1 => ZF=0 */
    TEST_ASSERT(!(flags & ZF_FLAG), "vtestpd all negative: ZF clear");
    /* NOT(a) AND b: NOT(neg) = pos sign bits = 0, AND with neg = 0 => CF=1 */
    TEST_ASSERT(flags & CF_FLAG, "vtestpd all negative: CF set (b subset of a signs)");
}

static void test_vtestps_256(void) {
    ymm_t a = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t b = { .f32 = {1,2,3,4,5,6,7,8} };
    uint64_t flags;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vtestps %2, %%ymm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags) : "m"(a), "m"(b) : "ymm0", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "vtestps 256 all positive: ZF set");
}

int main(void) {
    TEST_START("VTESTPS/VTESTPD instructions (AVX)");
    test_vtestps_all_positive();
    test_vtestps_mixed();
    test_vtestpd_basic();
    test_vtestps_256();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
