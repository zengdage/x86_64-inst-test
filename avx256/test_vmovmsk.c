/*
 * Test VMOVMSKPD/VMOVMSKPS
 * 256-bit extract sign mask bits to GPR
 * Compile: gcc -o test_vmovmsk avx256/test_vmovmsk.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
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

static void test_vmovmskpd_256(void) {
    TEST_START("VMOVMSKPD (256-bit)");
    /* all positive -> mask 0 */
    double a0[4] = {1.0, 2.0, 3.0, 4.0};
    int mask;
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovmskpd %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a0[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 0, "vmovmskpd all-positive mask=%d", mask);

    /* all negative -> mask 0xF */
    double a1[4] = {-1.0, -2.0, -3.0, -4.0};
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovmskpd %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a1[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 0xF, "vmovmskpd all-negative mask=%d", mask);

    /* mixed: neg,pos,neg,pos -> mask=0b0101=5 */
    double a2[4] = {-1.0, 2.0, -3.0, 4.0};
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovmskpd %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a2[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 5, "vmovmskpd mixed mask=%d", mask);

    /* negative zero */
    double a3[4] = {-0.0, 0.0, -0.0, 0.0};
    __asm__ volatile(
        "vmovupd %1, %%ymm0\n\t"
        "vmovmskpd %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a3[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 5, "vmovmskpd neg-zero mask=%d", mask);
}

static void test_vmovmskps_256(void) {
    TEST_START("VMOVMSKPS (256-bit)");
    /* all positive -> mask 0 */
    float a0[8] = {1,2,3,4,5,6,7,8};
    int mask;
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovmskps %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a0[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 0, "vmovmskps all-positive mask=%d", mask);

    /* all negative -> mask 0xFF */
    float a1[8] = {-1,-2,-3,-4,-5,-6,-7,-8};
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovmskps %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a1[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 0xFF, "vmovmskps all-negative mask=%d", mask);

    /* alternating: neg,pos,neg,pos,... -> 0b01010101=0x55 */
    float a2[8] = {-1,2,-3,4,-5,6,-7,8};
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovmskps %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a2[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 0x55, "vmovmskps alternating mask=0x%02x", mask);

    /* only element[7] negative -> bit7 = 0x80 */
    float a3[8] = {1,2,3,4,5,6,7,-8};
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovmskps %%ymm0, %0\n\t"
        : "=r"(mask) : "m"(a3[0]) : "ymm0"
    );
    TEST_ASSERT(mask == 0x80, "vmovmskps one-neg mask=0x%02x", mask);
}

static void test_vmovmskps_128(void) {
    TEST_START("VMOVMSKPS (128-bit)");
    static const struct {
        const char *name;
        xmm_t value;
        unsigned expected;
    } cases[] = {
        {"all positive", {.u32 = {
            UINT32_C(0x3f800000), UINT32_C(0x40000000),
            UINT32_C(0x40400000), UINT32_C(0x40800000)}}, 0x0},
        {"all negative", {.u32 = {
            UINT32_C(0xbf800000), UINT32_C(0xc0000000),
            UINT32_C(0xc0400000), UINT32_C(0xc0800000)}}, 0xf},
        {"alternating signs", {.u32 = {
            UINT32_C(0x80000000), UINT32_C(0x00000000),
            UINT32_C(0x80000000), UINT32_C(0x00000000)}}, 0x5},
        {"NaN and infinity", {.u32 = {
            UINT32_C(0x7fc12345), UINT32_C(0xffc54321),
            UINT32_C(0x7f800000), UINT32_C(0xff800000)}}, 0xa},
        {"subnormal and sign boundary", {.u32 = {
            UINT32_C(0x00000001), UINT32_C(0x80000001),
            UINT32_C(0x7fffffff), UINT32_C(0x80000000)}}, 0xa},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        unsigned mask;
        __asm__ volatile(
            "vmovdqu %1, %%xmm0\n\t"
            "vmovmskps %%xmm0, %0\n\t"
            : "=r"(mask) : "m"(cases[i].value) : "xmm0");
        TEST_ASSERT(mask == cases[i].expected,
                    "vmovmskps 128-bit %s: expected 0x%x, got 0x%x",
                    cases[i].name, cases[i].expected, mask);
    }
}

static void test_vmovmskps_special_values(void) {
    /* VMOVMSKPS extracts bit 31 only; it does not classify the value.  Keep
       the payloads here so NaNs, infinities, zeros, subnormals, and finite
       extrema exercise the same sign-bit path as ordinary numbers. */
    static const struct {
        const char *name;
        ymm_t value;
        unsigned expected;
    } cases[] = {
        {"positive zeros", {.u32 = {
            UINT32_C(0x00000000), UINT32_C(0x00000000),
            UINT32_C(0x00000000), UINT32_C(0x00000000),
            UINT32_C(0x00000000), UINT32_C(0x00000000),
            UINT32_C(0x00000000), UINT32_C(0x00000000)}}, 0x00},
        {"negative zeros", {.u32 = {
            UINT32_C(0x80000000), UINT32_C(0x80000000),
            UINT32_C(0x80000000), UINT32_C(0x80000000),
            UINT32_C(0x80000000), UINT32_C(0x80000000),
            UINT32_C(0x80000000), UINT32_C(0x80000000)}}, 0xff},
        {"infinities", {.u32 = {
            UINT32_C(0x7f800000), UINT32_C(0xff800000),
            UINT32_C(0x7f800000), UINT32_C(0xff800000),
            UINT32_C(0x7f800000), UINT32_C(0xff800000),
            UINT32_C(0x7f800000), UINT32_C(0xff800000)}}, 0xaa},
        {"NaNs", {.u32 = {
            UINT32_C(0x7fc00001), UINT32_C(0xffc00001),
            UINT32_C(0x7f800001), UINT32_C(0xff800001),
            UINT32_C(0x7fffffff), UINT32_C(0xffffffff),
            UINT32_C(0x7fc12345), UINT32_C(0xffc54321)}}, 0xaa},
        {"subnormal and finite extrema", {.u32 = {
            UINT32_C(0x00000001), UINT32_C(0x80000001),
            UINT32_C(0x007fffff), UINT32_C(0x807fffff),
            UINT32_C(0x7f7fffff), UINT32_C(0xff7fffff),
            UINT32_C(0x00800000), UINT32_C(0x80800000)}}, 0xaa},
        {"sign-bit boundary", {.u32 = {
            UINT32_C(0x7fffffff), UINT32_C(0x80000000),
            UINT32_C(0x00000000), UINT32_C(0xffffffff),
            UINT32_C(0x00000001), UINT32_C(0x80000001),
            UINT32_C(0x7f800001), UINT32_C(0xff800001)}}, 0xaa},
    };

    TEST_START("VMOVMSKPS special values and sign-bit boundaries");
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        unsigned mask;
        __asm__ volatile(
            "vmovdqu %1, %%ymm0\n\t"
            "vmovmskps %%ymm0, %0\n\t"
            : "=r"(mask) : "m"(cases[i].value) : "ymm0");
        TEST_ASSERT(mask == cases[i].expected,
                    "vmovmskps %s: expected 0x%02x, got 0x%02x",
                    cases[i].name, cases[i].expected, mask);
    }
}

static void test_vmovmsk_special_bits_and_zero_extension(void) {
    ymm_t ps = { .u32 = {
        UINT32_C(0x80000001), UINT32_C(0x7f800000),
        UINT32_C(0x7fc12345), UINT32_C(0x80000000),
        UINT32_C(0x00000001), UINT32_C(0xff800000),
        UINT32_C(0x7f800001), UINT32_C(0xffc54321)
    } };
    ymm_t pd = { .u64 = {
        UINT64_C(0xfff0000000000000), UINT64_C(0x7ff8000000000001),
        UINT64_C(0x0000000000000001), UINT64_C(0xfff0000000000001)
    } };
    uint64_t result;

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "vmovdqu %1, %%ymm0\n\t"
        "vmovmskps %%ymm0, %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result) : "m"(ps) : "rax", "ymm0"
    );
    TEST_ASSERT(result == UINT64_C(0x00000000000000a9),
                "vmovmskps special signs including endpoint lanes: %#" PRIx64, result);

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "vmovdqu %1, %%ymm0\n\t"
        "vmovmskpd %%ymm0, %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result) : "m"(pd) : "rax", "ymm0"
    );
    TEST_ASSERT(result == UINT64_C(0x0000000000000009),
                "vmovmskpd special signs including endpoint lanes: %#" PRIx64, result);
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vmovmskpd_256();
    test_vmovmskps_128();
    test_vmovmskps_256();
    test_vmovmskps_special_values();
    test_vmovmsk_special_bits_and_zero_extension();
    TEST_END();
}
