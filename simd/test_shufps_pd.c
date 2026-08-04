/*
 * test_shufps_pd.c - Test SHUFPS/SHUFPD instructions
 *
 * SHUFPS: Shuffle packed single-precision. Low 2 elements from dest, high 2 from src,
 *         selected by imm8 (2 bits per element).
 * SHUFPD: Shuffle packed double-precision. Bit 0 selects from dest, bit 1 from src.
 *
 * Compile: gcc -o test_shufps_pd simd/test_shufps_pd.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_shufps_identity(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t dst;

    /* 0xE4 = 11_10_01_00: dst[0]=a[0], dst[1]=a[1], dst[2]=b[2], dst[3]=b[3] */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "shufps $0xE4, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f, "shufps identity [0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 2.0f, "shufps identity [1]: got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 7.0f, "shufps identity [2]: got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 8.0f, "shufps identity [3]: got %f", dst.f32[3]);
}

static void test_shufps_broadcast(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t dst;

    /* 0x00: all select element 0 from each half */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "shufps $0x00, %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == 1.0f,
            "shufps broadcast [%d]: expected 1.0, got %f", i, dst.f32[i]);
    }
}

static void test_shufps_interleave(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {5.0f, 6.0f, 7.0f, 8.0f} };
    xmm_t dst;

    /* 0x88 = 10_00_10_00: dst[0]=a[0], dst[1]=a[2], dst[2]=b[0], dst[3]=b[2] */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "shufps $0x88, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f, "shufps interleave [0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 3.0f, "shufps interleave [1]: got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 5.0f, "shufps interleave [2]: got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 7.0f, "shufps interleave [3]: got %f", dst.f32[3]);
}

static void test_shufpd_basic(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {3.0, 4.0} };
    xmm_t dst;

    /* imm8=0: dst[0]=a[0], dst[1]=b[0] */
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "shufpd $0, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 1.0, "shufpd $0 [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 3.0, "shufpd $0 [1]: got %f", dst.f64[1]);
}

static void test_shufpd_swap(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {3.0, 4.0} };
    xmm_t dst;

    /* imm8=3: dst[0]=a[1], dst[1]=b[1] */
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "shufpd $3, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 2.0, "shufpd $3 [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 4.0, "shufpd $3 [1]: got %f", dst.f64[1]);
}

static void test_shufpd_cross(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {3.0, 4.0} };
    xmm_t dst;

    /* imm8=1: dst[0]=a[1], dst[1]=b[0] */
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "shufpd $1, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == 2.0, "shufpd $1 [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 3.0, "shufpd $1 [1]: got %f", dst.f64[1]);
}

static void test_shufpd_high_immediate_bits(void) {
    xmm_t a = { .u64 = {UINT64_C(0x0123456789abcdef), UINT64_C(0x1111111111111111)} };
    xmm_t b = { .u64 = {UINT64_C(0x2222222222222222), UINT64_C(0xfedcba9876543210)} };
    xmm_t dst;

    /* SHUFPD only consumes imm8[1:0], so 0xff is equivalent to 3. */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "shufpd $0xff, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == a.u64[1], "shufpd $0xff: low lane selects a[1]");
    TEST_ASSERT(dst.u64[1] == b.u64[1], "shufpd $0xff: high lane selects b[1]");
}

int main(void) {
    TEST_START("SHUFPS/SHUFPD instructions");
    test_shufps_identity();
    test_shufps_broadcast();
    test_shufps_interleave();
    test_shufpd_basic();
    test_shufpd_swap();
    test_shufpd_cross();
    test_shufpd_high_immediate_bits();
    TEST_END();
}
