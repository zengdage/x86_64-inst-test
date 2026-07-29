/*
 * test_ucomiss.c - Test SSE UCOMISS/COMISS instructions
 *
 * UCOMISS: Unordered compare scalar single-precision, set EFLAGS (ZF, PF, CF).
 *   - ST > src:   ZF=0, PF=0, CF=0
 *   - ST < src:   ZF=0, PF=0, CF=1
 *   - ST == src:  ZF=1, PF=0, CF=0
 *   - Unordered:  ZF=1, PF=1, CF=1
 * COMISS: Same as UCOMISS but raises #IA for any NaN (UCOMISS only for SNaN).
 *
 * Compile: gcc -o test_ucomiss float/test_ucomiss.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static void test_ucomiss_greater(void) {
    xmm_t a, b;
    uint64_t flags;

    a.f32[0] = 10.0f;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomiss 10>5: ZF=0");
    TEST_ASSERT(!(flags & PF_FLAG), "ucomiss 10>5: PF=0");
    TEST_ASSERT(!(flags & CF_FLAG), "ucomiss 10>5: CF=0");
}

static void test_ucomiss_less(void) {
    xmm_t a, b;
    uint64_t flags;

    a.f32[0] = 3.0f;
    b.f32[0] = 7.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomiss 3<7: ZF=0");
    TEST_ASSERT(!(flags & PF_FLAG), "ucomiss 3<7: PF=0");
    TEST_ASSERT(flags & CF_FLAG, "ucomiss 3<7: CF=1");
}

static void test_ucomiss_equal(void) {
    xmm_t a, b;
    uint64_t flags;

    a.f32[0] = 5.0f;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomiss 5==5: ZF=1");
    TEST_ASSERT(!(flags & PF_FLAG), "ucomiss 5==5: PF=0");
    TEST_ASSERT(!(flags & CF_FLAG), "ucomiss 5==5: CF=0");
}

static void test_ucomiss_unordered(void) {
    xmm_t a, b;
    uint64_t flags;

    /* NaN vs normal */
    a.f32[0] = NAN;
    b.f32[0] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomiss NaN: ZF=1");
    TEST_ASSERT(flags & PF_FLAG, "ucomiss NaN: PF=1");
    TEST_ASSERT(flags & CF_FLAG, "ucomiss NaN: CF=1");

    /* NaN vs NaN */
    a.f32[0] = NAN;
    b.f32[0] = NAN;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomiss NaN,NaN: ZF=1");
    TEST_ASSERT(flags & PF_FLAG, "ucomiss NaN,NaN: PF=1");
    TEST_ASSERT(flags & CF_FLAG, "ucomiss NaN,NaN: CF=1");
}

static void test_ucomiss_special(void) {
    xmm_t a, b;
    uint64_t flags;

    /* +0 == -0 */
    a.f32[0] = 0.0f;
    b.f32[0] = -0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "ucomiss +0==-0: ZF=1");
    TEST_ASSERT(!(flags & CF_FLAG), "ucomiss +0==-0: CF=0");

    /* Inf > any finite */
    a.f32[0] = INFINITY;
    b.f32[0] = FLT_MAX;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "ucomiss inf>FLT_MAX: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomiss inf>FLT_MAX: ZF=0");

    /* -Inf < any finite */
    a.f32[0] = -INFINITY;
    b.f32[0] = -FLT_MAX;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "ucomiss -inf<-FLT_MAX: CF=1");

    /* Denormal */
    a.f32[0] = FLT_MIN / 2.0f;
    b.f32[0] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "ucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "ucomiss denorm>0: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomiss denorm>0: ZF=0");
}

static void test_ucomiss_mem(void) {
    xmm_t a;
    float mem_val = 5.0f;
    uint64_t flags;

    a.f32[0] = 10.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "ucomiss %2, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(mem_val)
        : "xmm0", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "ucomiss xmm,mem 10>5: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "ucomiss xmm,mem 10>5: ZF=0");
}

static void test_comiss(void) {
    xmm_t a, b;
    uint64_t flags;

    /* Greater */
    a.f32[0] = 10.0f;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "comiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "comiss 10>5: CF=0");
    TEST_ASSERT(!(flags & ZF_FLAG), "comiss 10>5: ZF=0");

    /* Less */
    a.f32[0] = 3.0f;
    b.f32[0] = 7.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "comiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "comiss 3<7: CF=1");

    /* Equal */
    a.f32[0] = 5.0f;
    b.f32[0] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "comiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1", "cc"
    );
    TEST_ASSERT(flags & ZF_FLAG, "comiss 5==5: ZF=1");
    TEST_ASSERT(!(flags & CF_FLAG), "comiss 5==5: CF=0");
}

int main(void) {
    TEST_START("UCOMISS/COMISS instructions");
    test_ucomiss_greater();
    test_ucomiss_less();
    test_ucomiss_equal();
    test_ucomiss_unordered();
    test_ucomiss_special();
    test_ucomiss_mem();
    test_comiss();
    TEST_END();
}
