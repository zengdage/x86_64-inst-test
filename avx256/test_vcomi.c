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

static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}

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

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vcomisd();
    test_vcomiss();
    test_vucomisd();
    test_vucomiss();
    TEST_END();
}
