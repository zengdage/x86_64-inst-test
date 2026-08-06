/*
 * Test VCOMISD/VCOMISS/VUCOMISD/VUCOMISS
 * Scalar ordered/unordered compare, sets EFLAGS CF/ZF/PF
 * Compile: gcc -o test_vcomi avx256/test_vcomi.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}
#else
#define check_avx() 1
#endif

/* Returns EFLAGS after vcomisd xmm0, xmm1 */
static uint64_t do_vcomisd(double a, double b) {
    uint64_t flags;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vcomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    return flags;
}

static uint64_t do_vcomiss(float a, float b) {
    uint64_t flags;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vcomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    return flags;
}

static uint64_t do_vucomisd(double a, double b) {
    uint64_t flags;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vmovsd %2, %%xmm1\n\t"
        "vucomisd %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    return flags;
}

static uint64_t do_vucomiss(float a, float b) {
    uint64_t flags;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vmovss %2, %%xmm1\n\t"
        "vucomiss %%xmm1, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0","xmm1"
    );
    return flags;
}

/* mem operand forms */
static uint64_t do_vcomisd_mem(double a, double b) {
    uint64_t flags;
    __asm__ volatile(
        "vmovsd %1, %%xmm0\n\t"
        "vcomisd %2, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0"
    );
    return flags;
}

static uint64_t do_vcomiss_mem(float a, float b) {
    uint64_t flags;
    __asm__ volatile(
        "vmovss %1, %%xmm0\n\t"
        "vcomiss %2, %%xmm0\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        : "=r"(flags) : "m"(a), "m"(b) : "xmm0"
    );
    return flags;
}

#if ENABLE_MXCSR_CHECK
static void assert_vcomi_flags(uint64_t flags, int zf, int pf, int cf,
                               const char *name) {
    TEST_ASSERT(!!(flags & ZF_FLAG) == zf, "%s ZF=%d expected %d", name,
                !!(flags & ZF_FLAG), zf);
    TEST_ASSERT(!!(flags & PF_FLAG) == pf, "%s PF=%d expected %d", name,
                !!(flags & PF_FLAG), pf);
    TEST_ASSERT(!!(flags & CF_FLAG) == cf, "%s CF=%d expected %d", name,
                !!(flags & CF_FLAG), cf);
    TEST_ASSERT((flags & (OF_FLAG | SF_FLAG | AF_FLAG)) == 0,
                "%s clears OF/SF/AF: flags=%#" PRIx64, name, flags);
}
#endif

#if ENABLE_MXCSR_CHECK
static uint64_t run_vcomi_nan_sd(uint64_t bits, int unordered,
                                 uint32_t *exception_flags) {
    xmm_t value = { .u64 = {bits, 0} };
    xmm_t one = { .u64 = {UINT64_C(0x3ff0000000000000), 0} };
    uint32_t saved, csr;
    uint64_t flags;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    csr = (saved & ~UINT32_C(0x3f)) | UINT32_C(0x80);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    if (unordered) {
        __asm__ volatile (
            "vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\t"
            "vucomisd %%xmm1, %%xmm0\n\tpushfq\n\tpopq %0"
            : "=r"(flags) : "m"(value), "m"(one)
            : "xmm0", "xmm1", "cc");
    } else {
        __asm__ volatile (
            "vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\t"
            "vcomisd %%xmm1, %%xmm0\n\tpushfq\n\tpopq %0"
            : "=r"(flags) : "m"(value), "m"(one)
            : "xmm0", "xmm1", "cc");
    }
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
    *exception_flags = csr & UINT32_C(0x3f);
    return flags;
}

static uint64_t run_vcomi_nan_ss(uint32_t bits, int unordered,
                                 uint32_t *exception_flags) {
    xmm_t value = { .u32 = {bits, 0, 0, 0} };
    xmm_t one = { .u32 = {UINT32_C(0x3f800000), 0, 0, 0} };
    uint32_t saved, csr;
    uint64_t flags;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    csr = (saved & ~UINT32_C(0x3f)) | UINT32_C(0x80);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    if (unordered) {
        __asm__ volatile (
            "vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\t"
            "vucomiss %%xmm1, %%xmm0\n\tpushfq\n\tpopq %0"
            : "=r"(flags) : "m"(value), "m"(one)
            : "xmm0", "xmm1", "cc");
    } else {
        __asm__ volatile (
            "vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\t"
            "vcomiss %%xmm1, %%xmm0\n\tpushfq\n\tpopq %0"
            : "=r"(flags) : "m"(value), "m"(one)
            : "xmm0", "xmm1", "cc");
    }
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
    *exception_flags = csr & UINT32_C(0x3f);
    return flags;
}
#endif

static void test_vcomisd(void) {
    TEST_START("VCOMISD");
    uint64_t f;
    /* a > b: ZF=0, CF=0 */
    f = do_vcomisd(5.0, 3.0);
    TEST_ASSERT(!(f & ZF_FLAG) && !(f & CF_FLAG), "vcomisd a>b: ZF=0 CF=0, flags=%016llx", (unsigned long long)f);
    /* a < b: ZF=0, CF=1 */
    f = do_vcomisd(3.0, 5.0);
    TEST_ASSERT(!(f & ZF_FLAG) && (f & CF_FLAG), "vcomisd a<b: ZF=0 CF=1, flags=%016llx", (unsigned long long)f);
    /* a == b: ZF=1, CF=0 */
    f = do_vcomisd(4.0, 4.0);
    TEST_ASSERT((f & ZF_FLAG) && !(f & CF_FLAG), "vcomisd a==b: ZF=1 CF=0, flags=%016llx", (unsigned long long)f);
    /* NaN: ZF=1, CF=1, PF=1 */
    double nan = NAN;
    f = do_vcomisd(nan, 1.0);
    TEST_ASSERT((f & ZF_FLAG) && (f & CF_FLAG) && (f & PF_FLAG),
        "vcomisd NaN: ZF=1 CF=1 PF=1, flags=%016llx", (unsigned long long)f);
    /* mem form */
    f = do_vcomisd_mem(5.0, 3.0);
    TEST_ASSERT(!(f & ZF_FLAG) && !(f & CF_FLAG), "vcomisd mem a>b, flags=%016llx", (unsigned long long)f);
}

static void test_vcomiss(void) {
    TEST_START("VCOMISS");
    uint64_t f;
    f = do_vcomiss(5.0f, 3.0f);
    TEST_ASSERT(!(f & ZF_FLAG) && !(f & CF_FLAG), "vcomiss a>b: ZF=0 CF=0, flags=%016llx", (unsigned long long)f);
    f = do_vcomiss(3.0f, 5.0f);
    TEST_ASSERT(!(f & ZF_FLAG) && (f & CF_FLAG), "vcomiss a<b: ZF=0 CF=1, flags=%016llx", (unsigned long long)f);
    f = do_vcomiss(4.0f, 4.0f);
    TEST_ASSERT((f & ZF_FLAG) && !(f & CF_FLAG), "vcomiss a==b: ZF=1 CF=0, flags=%016llx", (unsigned long long)f);
    float nan = NAN;
    f = do_vcomiss(nan, 1.0f);
    TEST_ASSERT((f & ZF_FLAG) && (f & CF_FLAG) && (f & PF_FLAG),
        "vcomiss NaN: ZF=1 CF=1 PF=1, flags=%016llx", (unsigned long long)f);
    /* mem form */
    f = do_vcomiss_mem(5.0f, 3.0f);
    TEST_ASSERT(!(f & ZF_FLAG) && !(f & CF_FLAG), "vcomiss mem a>b, flags=%016llx", (unsigned long long)f);
}

static void test_vucomisd(void) {
    TEST_START("VUCOMISD");
    uint64_t f;
    f = do_vucomisd(5.0, 3.0);
    TEST_ASSERT(!(f & ZF_FLAG) && !(f & CF_FLAG), "vucomisd a>b: ZF=0 CF=0, flags=%016llx", (unsigned long long)f);
    f = do_vucomisd(3.0, 5.0);
    TEST_ASSERT(!(f & ZF_FLAG) && (f & CF_FLAG), "vucomisd a<b: ZF=0 CF=1, flags=%016llx", (unsigned long long)f);
    f = do_vucomisd(4.0, 4.0);
    TEST_ASSERT((f & ZF_FLAG) && !(f & CF_FLAG), "vucomisd a==b: ZF=1 CF=0, flags=%016llx", (unsigned long long)f);
    /* unordered (NaN): ZF=1, CF=1, PF=1 */
    double nan = NAN;
    f = do_vucomisd(nan, 1.0);
    TEST_ASSERT((f & ZF_FLAG) && (f & CF_FLAG) && (f & PF_FLAG),
        "vucomisd NaN: ZF=1 CF=1 PF=1, flags=%016llx", (unsigned long long)f);
}

static void test_vucomiss(void) {
    TEST_START("VUCOMISS");
    uint64_t f;
    f = do_vucomiss(5.0f, 3.0f);
    TEST_ASSERT(!(f & ZF_FLAG) && !(f & CF_FLAG), "vucomiss a>b: ZF=0 CF=0, flags=%016llx", (unsigned long long)f);
    f = do_vucomiss(3.0f, 5.0f);
    TEST_ASSERT(!(f & ZF_FLAG) && (f & CF_FLAG), "vucomiss a<b: ZF=0 CF=1, flags=%016llx", (unsigned long long)f);
    f = do_vucomiss(4.0f, 4.0f);
    TEST_ASSERT((f & ZF_FLAG) && !(f & CF_FLAG), "vucomiss a==b: ZF=1 CF=0, flags=%016llx", (unsigned long long)f);
    float nan = NAN;
    f = do_vucomiss(nan, 1.0f);
    TEST_ASSERT((f & ZF_FLAG) && (f & CF_FLAG) && (f & PF_FLAG),
        "vucomiss NaN: ZF=1 CF=1 PF=1, flags=%016llx", (unsigned long long)f);
}

#if ENABLE_MXCSR_CHECK
static void test_vcomi_nan_exceptions_and_flag_clearing(void) {
    assert_vcomi_flags(do_vcomisd(5.0, 3.0), 0, 0, 0,
                       "vcomisd finite greater");
    assert_vcomi_flags(do_vcomiss(3.0f, 5.0f), 0, 0, 1,
                       "vcomiss finite less");
    assert_vcomi_flags(do_vucomisd(4.0, 4.0), 1, 0, 0,
                       "vucomisd finite equal");

    uint32_t exceptions;
    uint64_t flags = run_vcomi_nan_sd(UINT64_C(0x7ff8000000000001), 0,
                                      &exceptions);
    assert_vcomi_flags(flags, 1, 1, 1, "vcomisd double QNaN");
    TEST_ASSERT(exceptions & 1u, "vcomisd QNaN sets MXCSR invalid");

    flags = run_vcomi_nan_sd(UINT64_C(0x7ff8000000000001), 1,
                             &exceptions);
    assert_vcomi_flags(flags, 1, 1, 1, "vucomisd double QNaN");
    TEST_ASSERT(!(exceptions & 1u), "vucomisd QNaN leaves MXCSR invalid clear");

    flags = run_vcomi_nan_sd(UINT64_C(0x7ff0000000000001), 1,
                             &exceptions);
    assert_vcomi_flags(flags, 1, 1, 1, "vucomisd double SNaN");
    TEST_ASSERT(exceptions & 1u, "vucomisd SNaN sets MXCSR invalid");

    flags = run_vcomi_nan_ss(UINT32_C(0x7fc00001), 0, &exceptions);
    assert_vcomi_flags(flags, 1, 1, 1, "vcomiss float QNaN");
    TEST_ASSERT(exceptions & 1u, "vcomiss QNaN sets MXCSR invalid");

    flags = run_vcomi_nan_ss(UINT32_C(0x7fc00001), 1, &exceptions);
    assert_vcomi_flags(flags, 1, 1, 1, "vucomiss float QNaN");
    TEST_ASSERT(!(exceptions & 1u), "vucomiss QNaN leaves MXCSR invalid clear");

    flags = run_vcomi_nan_ss(UINT32_C(0x7f800001), 1, &exceptions);
    assert_vcomi_flags(flags, 1, 1, 1, "vucomiss float SNaN");
    TEST_ASSERT(exceptions & 1u, "vucomiss SNaN sets MXCSR invalid");
}
#endif

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vcomisd();
    test_vcomiss();
    test_vucomisd();
    test_vucomiss();
#if ENABLE_MXCSR_CHECK
    test_vcomi_nan_exceptions_and_flag_clearing();
#endif
    TEST_END();
}
