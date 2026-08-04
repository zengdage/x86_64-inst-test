/*
 * test_movmskps_pd.c - Test MOVMSKPS and MOVMSKPD instructions
 *
 * MOVMSKPS: Extract sign bits from 4 single-precision floats into GPR.
 * MOVMSKPD: Extract sign bits from 2 double-precision floats into GPR.
 * Result is a bitmask in a general-purpose register.
 *
 * Compile: gcc -o test_movmskps_pd simd/test_movmskps_pd.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movmskps_all_positive(void) {
    xmm_t src = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    int32_t result;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movmskps %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == 0, "movmskps all positive: expected 0, got %d", result);
}

static void test_movmskps_all_negative(void) {
    xmm_t src = { .f32 = { -1.0f, -2.0f, -3.0f, -4.0f } };
    int32_t result;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movmskps %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == 0xF, "movmskps all negative: expected 0xF, got 0x%x", result);
}

static void test_movmskps_mixed(void) {
    xmm_t src = { .f32 = { 1.0f, -2.0f, 3.0f, -4.0f } };
    int32_t result;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movmskps %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    /* bit0=sign(f32[0])=0, bit1=sign(f32[1])=1, bit2=sign(f32[2])=0, bit3=sign(f32[3])=1 */
    TEST_ASSERT(result == 0xA, "movmskps mixed: expected 0xA, got 0x%x", result);
}

static void test_movmskps_neg_zero(void) {
    xmm_t src = { .f32 = { 0.0f, -0.0f, 0.0f, -0.0f } };
    int32_t result;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movmskps %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == 0xA, "movmskps -0.0: expected 0xA, got 0x%x", result);
}

static void test_movmskpd_all_positive(void) {
    xmm_t src = { .f64 = { 1.0, 2.0 } };
    int32_t result;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movmskpd %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == 0, "movmskpd all positive: expected 0, got %d", result);
}

static void test_movmskpd_all_negative(void) {
    xmm_t src = { .f64 = { -1.0, -2.0 } };
    int32_t result;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movmskpd %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == 3, "movmskpd all negative: expected 3, got %d", result);
}

static void test_movmskpd_mixed(void) {
    xmm_t src = { .f64 = { -1.0, 2.0 } };
    int32_t result;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movmskpd %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == 1, "movmskpd mixed(-,+): expected 1, got %d", result);
}

static void test_movmskpd_neg_zero(void) {
    xmm_t src = { .f64 = { -0.0, 0.0 } };
    int32_t result;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movmskpd %%xmm0, %0"
        : "=r"(result)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(result == 1, "movmskpd -0.0: expected 1, got %d", result);
}

static void test_movmsk_special_bits_and_zero_extension(void) {
    xmm_t ps = { .u32 = {
        UINT32_C(0x80000001), /* negative minimum subnormal */
        UINT32_C(0x7f800000), /* +Inf */
        UINT32_C(0x7fc12345), /* positive QNaN */
        UINT32_C(0xffc54321)  /* negative QNaN */
    } };
    xmm_t pd = { .u64 = {
        UINT64_C(0x7ff0000000000001), /* positive SNaN */
        UINT64_C(0xfff0000000000000)  /* -Inf */
    } };
    uint64_t result;

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "movdqa %1, %%xmm0\n\t"
        "movmskps %%xmm0, %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result) : "m"(ps) : "rax", "xmm0"
    );
    TEST_ASSERT(result == UINT64_C(0x0000000000000009),
                "movmskps special sign bits and eax zero extension: %#" PRIx64, result);

    __asm__ volatile (
        "movq $-1, %%rax\n\t"
        "movdqa %1, %%xmm0\n\t"
        "movmskpd %%xmm0, %%eax\n\t"
        "movq %%rax, %0"
        : "=r"(result) : "m"(pd) : "rax", "xmm0"
    );
    TEST_ASSERT(result == UINT64_C(0x0000000000000002),
                "movmskpd NaN/Inf sign bits and eax zero extension: %#" PRIx64, result);
}

int main(void) {
    TEST_START("MOVMSKPS/MOVMSKPD instructions");
    test_movmskps_all_positive();
    test_movmskps_all_negative();
    test_movmskps_mixed();
    test_movmskps_neg_zero();
    test_movmskpd_all_positive();
    test_movmskpd_all_negative();
    test_movmskpd_mixed();
    test_movmskpd_neg_zero();
    test_movmsk_special_bits_and_zero_extension();
    TEST_END();
}
