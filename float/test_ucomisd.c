/*
 * test_ucomisd.c - Test SSE2 UCOMISD/COMISD instructions
 *
 * UCOMISD: Unordered compare scalar double-precision, set EFLAGS (ZF, PF, CF).
 *   - ST > src:   ZF=0, PF=0, CF=0
 *   - ST < src:   ZF=0, PF=0, CF=1
 *   - ST == src:  ZF=1, PF=0, CF=0
 *   - Unordered:  ZF=1, PF=1, CF=1
 * COMISD: Same as UCOMISD but raises #IA for any NaN.
 *
 * Compile: gcc -o test_ucomisd float/test_ucomisd.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_ucomisd_greater(void) {
    xmm_t a, b;
    uint64_t flags;

    a.f64[0] = 10.0;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomisd 10>5: ZF=0");
    TEST_ASSERT(!(flags & PF_FLAG), "ucomisd 10>5: PF=0");
    TEST_ASSERT(!(flags & CF_FLAG), "ucomisd 10>5: CF=0");
}

static void test_ucomisd_less(void) {
    xmm_t a, b;
    uint64_t flags;

    a.f64[0] = 3.0;
    b.f64[0] = 7.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomisd 3<7: ZF=0");
    TEST_ASSERT(!(flags & PF_FLAG), "ucomisd 3<7: PF=0");
    TEST_ASSERT(flags & CF_FLAG, "ucomisd 3<7: CF=1");
}

static void test_ucomisd_equal(void) {
    xmm_t a, b;
    uint64_t flags;

    a.f64[0] = 5.0;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomisd 5==5: ZF=1");
    TEST_ASSERT(!(flags & PF_FLAG), "ucomisd 5==5: PF=0");
    TEST_ASSERT(!(flags & CF_FLAG), "ucomisd 5==5: CF=0");
}

static void test_ucomisd_unordered(void) {
    xmm_t a, b;
    uint64_t flags;

    /* NaN vs normal */
    a.f64[0] = NAN;
    b.f64[0] = 1.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomisd NaN: ZF=1");
    TEST_ASSERT(flags & PF_FLAG, "ucomisd NaN: PF=1");
    TEST_ASSERT(flags & CF_FLAG, "ucomisd NaN: CF=1");

    /* Normal vs NaN */
    a.f64[0] = 1.0;
    b.f64[0] = NAN;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomisd 1,NaN: ZF=1");
    TEST_ASSERT(flags & PF_FLAG, "ucomisd 1,NaN: PF=1");
    TEST_ASSERT(flags & CF_FLAG, "ucomisd 1,NaN: CF=1");
}

static void test_ucomisd_special(void) {
    xmm_t a, b;
    uint64_t flags;

    /* +0 == -0 */
    a.f64[0] = 0.0;
    b.f64[0] = -0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomisd +0==-0: ZF=1");
    TEST_ASSERT(!(flags & CF_FLAG), "ucomisd +0==-0: CF=0");

    /* Inf == Inf */
    a.f64[0] = INFINITY;
    b.f64[0] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomisd inf==inf: ZF=1");

    /* Inf > -Inf */
    a.f64[0] = INFINITY;
    b.f64[0] = -INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "ucomisd inf>-inf: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomisd inf>-inf: ZF=0");

    /* Denormal > 0 */
    a.f64[0] = DBL_MIN / 2.0;
    b.f64[0] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "ucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "ucomisd denorm>0: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomisd denorm>0: ZF=0");
}

static void test_ucomisd_mem(void) {
    xmm_t a;
    double mem_val = 5.0;
    uint64_t flags;

    a.f64[0] = 10.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "ucomisd %2, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(mem_val)
        : "xmm0", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "ucomisd xmm,mem 10>5: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomisd xmm,mem 10>5: ZF=0");
}

static void test_comisd(void) {
    xmm_t a, b;
    uint64_t flags;

    /* Greater */
    a.f64[0] = 10.0;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "comisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "comisd 10>5: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "comisd 10>5: ZF=0");

    /* Less */
    a.f64[0] = 3.0;
    b.f64[0] = 7.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "comisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "comisd 3<7: CF=1");

    /* Equal */
    a.f64[0] = 5.0;
    b.f64[0] = 5.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "comisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "comisd 5==5: ZF=1");
    TEST_ASSERT(!(flags & CF_FLAG), "comisd 5==5: CF=0");
}

int main(void) {
    TEST_START("UCOMISD/COMISD instructions");
    test_ucomisd_greater();
    test_ucomisd_less();
    test_ucomisd_equal();
    test_ucomisd_unordered();
    test_ucomisd_special();
    test_ucomisd_mem();
    test_comisd();
    TEST_END();
}
