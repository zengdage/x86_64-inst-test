/*
 * test_vgather.c - Test VGATHERDPS/VGATHERDPD/VGATHERQPS/VGATHERQPD (AVX2)
 *
 * Gather instructions load elements from memory using a base address and
 * vector of signed dword/qword indices. A mask register controls which
 * elements are actually loaded.
 *
 * VGATHERDPS: Gather floats using dword indices.
 * VGATHERDPD: Gather doubles using dword indices.
 * VGATHERQPS: Gather floats using qword indices.
 * VGATHERQPD: Gather doubles using qword indices.
 *
 * Compile: gcc -o test_vgather simd/test_vgather.c -O0 -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vgatherdps_128(void) {
    float table[16] = {0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150};
    xmm_t indices = { .i32 = {0, 3, 7, 15} };  /* scale=4 for float */
    xmm_t mask = { .u32 = {0x80000000, 0x80000000, 0x80000000, 0x80000000} };
    xmm_t dst;

    __asm__ volatile (
        "vmovdqa %2, %%xmm1\n\t"    /* indices */
        "vmovdqa %3, %%xmm2\n\t"    /* mask (all set) */
        "vxorps %%xmm0, %%xmm0, %%xmm0\n\t"
        "vgatherdps %%xmm2, (%1, %%xmm1, 4), %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "r"(table), "m"(indices), "m"(mask)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(dst.f32[0] == 0.0f, "vgatherdps [0]: expected 0, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 30.0f, "vgatherdps [1]: expected 30, got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 70.0f, "vgatherdps [2]: expected 70, got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 150.0f, "vgatherdps [3]: expected 150, got %f", dst.f32[3]);
}

static void test_vgatherdps_partial_mask(void) {
    float table[8] = {100,200,300,400,500,600,700,800};
    xmm_t indices = { .i32 = {0, 1, 2, 3} };
    /* Only load elements 0 and 2 */
    xmm_t mask = { .u32 = {0x80000000, 0x00000000, 0x80000000, 0x00000000} };
    xmm_t init = { .f32 = {-1.0f, -1.0f, -1.0f, -1.0f} };
    xmm_t dst;

    __asm__ volatile (
        "vmovaps %4, %%xmm0\n\t"    /* initial values */
        "vmovdqa %2, %%xmm1\n\t"
        "vmovdqa %3, %%xmm2\n\t"
        "vgatherdps %%xmm2, (%1, %%xmm1, 4), %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "r"(table), "m"(indices), "m"(mask), "m"(init)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(dst.f32[0] == 100.0f, "vgatherdps partial [0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == -1.0f, "vgatherdps partial [1] unchanged: got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 300.0f, "vgatherdps partial [2]: got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == -1.0f, "vgatherdps partial [3] unchanged: got %f", dst.f32[3]);
}

static void test_vgatherdpd_128(void) {
    double table[8] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8};
    xmm_t indices = { .i32 = {2, 5, 0, 0} };  /* only 2 doubles in 128-bit */
    xmm_t mask = { .u64 = {0x8000000000000000ULL, 0x8000000000000000ULL} };
    xmm_t dst;

    __asm__ volatile (
        "vmovdqa %2, %%xmm1\n\t"
        "vmovdqa %3, %%xmm2\n\t"
        "vxorpd %%xmm0, %%xmm0, %%xmm0\n\t"
        "vgatherdpd %%xmm2, (%1, %%xmm1, 8), %%xmm0\n\t"
        "vmovapd %%xmm0, %0"
        : "=m"(dst) : "r"(table), "m"(indices), "m"(mask)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(dst.f64[0] == 3.3, "vgatherdpd [0]: expected 3.3, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 6.6, "vgatherdpd [1]: expected 6.6, got %f", dst.f64[1]);
}

static void test_vgatherqps_128(void) {
    float table[8] = {10,20,30,40,50,60,70,80};
    xmm_t indices = { .i64 = {3, 6} };
    xmm_t mask_full = { .u32 = {0x80000000, 0x80000000, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "vmovdqa %3, %%xmm2\n\t"
        "vmovdqa %2, %%xmm1\n\t"
        "vxorps %%xmm0, %%xmm0, %%xmm0\n\t"
        "vgatherqps %%xmm2, (%1, %%xmm1, 4), %%xmm0\n\t"
        "vmovaps %%xmm0, %0"
        : "=m"(dst) : "r"(table), "m"(indices), "m"(mask_full)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(dst.f32[0] == 40.0f, "vgatherqps [0]: expected 40, got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 70.0f, "vgatherqps [1]: expected 70, got %f", dst.f32[1]);
}

static void test_vgatherqpd_128(void) {
    double table[8] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    xmm_t indices = { .i64 = {1, 4} };
    xmm_t mask = { .u64 = {0x8000000000000000ULL, 0x8000000000000000ULL} };
    xmm_t dst;

    __asm__ volatile (
        "vmovdqa %2, %%xmm1\n\t"
        "vmovdqa %3, %%xmm2\n\t"
        "vxorpd %%xmm0, %%xmm0, %%xmm0\n\t"
        "vgatherqpd %%xmm2, (%1, %%xmm1, 8), %%xmm0\n\t"
        "vmovapd %%xmm0, %0"
        : "=m"(dst) : "r"(table), "m"(indices), "m"(mask)
        : "xmm0", "xmm1", "xmm2"
    );
    TEST_ASSERT(dst.f64[0] == 2.0, "vgatherqpd [0]: expected 2.0, got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 5.0, "vgatherqpd [1]: expected 5.0, got %f", dst.f64[1]);
}

int main(void) {
    TEST_START("VGATHERDPS/VGATHERDPD/VGATHERQPS/VGATHERQPD instructions (AVX2)");
    test_vgatherdps_128();
    test_vgatherdps_partial_mask();
    test_vgatherdpd_128();
    test_vgatherqps_128();
    test_vgatherqpd_128();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
