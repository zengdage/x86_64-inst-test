/*
 * test_movaps_movups.c - Test MOVAPS/MOVUPS/MOVAPD/MOVUPD instructions
 *
 * MOVAPS: Move aligned packed single-precision (128-bit, 16-byte aligned).
 * MOVUPS: Move unaligned packed single-precision (128-bit, no alignment req).
 * MOVAPD: Move aligned packed double-precision (128-bit, 16-byte aligned).
 * MOVUPD: Move unaligned packed double-precision (128-bit, no alignment req).
 *
 * Compile: gcc -o test_movaps_movups simd/test_movaps_movups.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movaps_xmm_to_xmm(void) {
    xmm_t src = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %%xmm0, %%xmm1\n\t"
        "movaps %%xmm1, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0", "xmm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == src.f32[i],
            "movaps xmm->xmm [%d]: expected %f, got %f", i, src.f32[i], dst.f32[i]);
    }
}

static void test_movaps_mem_roundtrip(void) {
    xmm_t src = { .f32 = { -1.5f, 0.0f, 3.14f, -100.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == src.f32[i],
            "movaps mem roundtrip [%d]: expected %f, got %f", i, src.f32[i], dst.f32[i]);
    }
}

static void test_movups_unaligned(void) {
    uint8_t buf[48] __attribute__((aligned(16)));
    float vals[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
    memcpy(buf + 3, vals, 16);

    xmm_t dst;
    __asm__ volatile (
        "movups %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst)
        : "m"(buf[3])
        : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == vals[i],
            "movups unaligned load [%d]: expected %f, got %f", i, vals[i], dst.f32[i]);
    }
}

static void test_movups_store_unaligned(void) {
    xmm_t src = { .f32 = { 10.0f, 20.0f, 30.0f, 40.0f } };
    uint8_t buf[48] __attribute__((aligned(16)));
    memset(buf, 0, sizeof(buf));

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movups %%xmm0, %0"
        : "=m"(buf[5])
        : "m"(src)
        : "xmm0"
    );
    float result[4];
    memcpy(result, buf + 5, 16);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(result[i] == src.f32[i],
            "movups unaligned store [%d]: expected %f, got %f", i, src.f32[i], result[i]);
    }
}

static void test_movapd_xmm_to_xmm(void) {
    xmm_t src = { .f64 = { 1.23456789, -9.87654321 } };
    xmm_t dst;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %%xmm0, %%xmm1\n\t"
        "movapd %%xmm1, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0", "xmm1"
    );
    TEST_ASSERT(dst.f64[0] == src.f64[0] && dst.f64[1] == src.f64[1],
        "movapd xmm->xmm: data mismatch");
}

static void test_movapd_mem_roundtrip(void) {
    xmm_t src = { .f64 = { 0.0, -0.0 } };
    xmm_t dst;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == src.u64[0] && dst.u64[1] == src.u64[1],
        "movapd mem roundtrip: data mismatch (check -0.0 preservation)");
}

static void test_movupd_unaligned(void) {
    uint8_t buf[48] __attribute__((aligned(16)));
    double vals[2] = { 3.14159, 2.71828 };
    memcpy(buf + 7, vals, 16);

    xmm_t dst;
    __asm__ volatile (
        "movupd %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst)
        : "m"(buf[7])
        : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == vals[0] && dst.f64[1] == vals[1],
        "movupd unaligned load: data mismatch");
}

static void test_movups_xmm_to_xmm(void) {
    xmm_t src = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movups %1, %%xmm0\n\t"
        "movups %%xmm0, %%xmm1\n\t"
        "movups %%xmm1, %0"
        : "=m"(dst)
        : "m"(src)
        : "xmm0", "xmm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == src.f32[i],
            "movups xmm->xmm [%d]: expected %f, got %f", i, src.f32[i], dst.f32[i]);
    }
}

int main(void) {
    TEST_START("MOVAPS/MOVUPS/MOVAPD/MOVUPD instructions");
    test_movaps_xmm_to_xmm();
    test_movaps_mem_roundtrip();
    test_movups_unaligned();
    test_movups_store_unaligned();
    test_movapd_xmm_to_xmm();
    test_movapd_mem_roundtrip();
    test_movupd_unaligned();
    test_movups_xmm_to_xmm();
    TEST_END();
}
