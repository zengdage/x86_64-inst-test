/*
 * test_vmovaps_ups.c - Test VMOVAPS/VMOVUPS/VMOVAPD/VMOVUPD (256-bit AVX)
 *
 * VEX-encoded 256-bit versions of aligned/unaligned move instructions.
 * VMOVAPS/VMOVAPD require 32-byte alignment for 256-bit operations.
 * VMOVUPS/VMOVUPD have no alignment requirement.
 *
 * Compile: gcc -o test_vmovaps_ups simd/test_vmovaps_ups.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vmovaps_ymm(void) {
    ymm_t src = { .f32 = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmovaps %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == src.f32[i],
            "vmovaps ymm [%d]: expected %f, got %f", i, src.f32[i], dst.f32[i]);
    }
}

static void test_vmovups_unaligned(void) {
    uint8_t buf[80] __attribute__((aligned(32)));
    float vals[8] = {10.0f,20.0f,30.0f,40.0f,50.0f,60.0f,70.0f,80.0f};
    memcpy(buf + 5, vals, 32);

    ymm_t dst;
    __asm__ volatile (
        "vmovups %1, %%ymm0\n\t"
        "vmovaps %%ymm0, %0"
        : "=m"(dst) : "m"(buf[5]) : "ymm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == vals[i],
            "vmovups unaligned [%d]: expected %f, got %f", i, vals[i], dst.f32[i]);
    }
}

static void test_vmovapd_ymm(void) {
    ymm_t src = { .f64 = {1.1, 2.2, 3.3, 4.4} };
    ymm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vmovapd %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "ymm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f64[i] == src.f64[i],
            "vmovapd ymm [%d]: expected %f, got %f", i, src.f64[i], dst.f64[i]);
    }
}

static void test_vmovupd_unaligned(void) {
    uint8_t buf[80] __attribute__((aligned(32)));
    double vals[4] = {3.14, 2.71, 1.41, 1.73};
    memcpy(buf + 7, vals, 32);

    ymm_t dst;
    __asm__ volatile (
        "vmovupd %1, %%ymm0\n\t"
        "vmovapd %%ymm0, %0"
        : "=m"(dst) : "m"(buf[7]) : "ymm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f64[i] == vals[i],
            "vmovupd unaligned [%d]: expected %f, got %f", i, vals[i], dst.f64[i]);
    }
}

static void test_vmovaps_store_ymm(void) {
    ymm_t src = { .f32 = {-1.0f,-2.0f,-3.0f,-4.0f,-5.0f,-6.0f,-7.0f,-8.0f} };
    ymm_t dst;
    memset(&dst, 0, sizeof(dst));

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmovaps %%ymm0, %0"
        : "=m"(dst) : "m"(src) : "ymm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == src.f32[i],
            "vmovaps store ymm [%d]: expected %f, got %f", i, src.f32[i], dst.f32[i]);
    }
}

static void test_vmovups_store_unaligned(void) {
    ymm_t src = { .f32 = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f} };
    uint8_t buf[80] __attribute__((aligned(32)));
    memset(buf, 0, sizeof(buf));

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vmovups %%ymm0, %0"
        : "=m"(buf[3]) : "m"(src) : "ymm0"
    );
    float result[8];
    memcpy(result, buf + 3, 32);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(result[i] == src.f32[i],
            "vmovups store unaligned [%d]: expected %f, got %f", i, src.f32[i], result[i]);
    }
}

int main(void) {
    TEST_START("VMOVAPS/VMOVUPS/VMOVAPD/VMOVUPD instructions (AVX 256-bit)");
    test_vmovaps_ymm();
    test_vmovups_unaligned();
    test_vmovapd_ymm();
    test_vmovupd_unaligned();
    test_vmovaps_store_ymm();
    test_vmovups_store_unaligned();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
