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
#include <sys/mman.h>
#include <unistd.h>

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

static void test_vgather_boundaries(void) {
    float table[24];
    for (int i = 0; i < 24; i++) table[i] = (float)(1000 + i);
    xmm_t indices = { .i32 = {-2, 0, 0, 3} };
    xmm_t mask = { .u32 = {0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u} };
    xmm_t dst, mask_after;
    float *base = &table[8];
    __asm__ volatile (
        "vmovdqa %3,%%xmm1\n\tvmovdqa %4,%%xmm2\n\tvxorps %%xmm0,%%xmm0,%%xmm0\n\t"
        "vgatherdps %%xmm2,(%2,%%xmm1,4),%%xmm0\n\tvmovaps %%xmm0,%0\n\tvmovdqa %%xmm2,%1"
        : "=m"(dst), "=m"(mask_after) : "r"(base), "m"(indices), "m"(mask)
        : "xmm0", "xmm1", "xmm2", "memory");
    TEST_ASSERT(dst.f32[0] == table[6] && dst.f32[1] == table[8] && dst.f32[2] == table[8] && dst.f32[3] == table[11],
        "vgatherdps negative and repeated indices");
    for (int i = 0; i < 4; i++) TEST_ASSERT(mask_after.u32[i] == 0, "vgatherdps clears successful mask lane %d", i);

    indices.i32[0] = 0; indices.i32[1] = 4; indices.i32[2] = 8; indices.i32[3] = 12;
    __asm__ volatile (
        "vmovdqa %2,%%xmm1\n\tvmovdqa %3,%%xmm2\n\tvxorps %%xmm0,%%xmm0,%%xmm0\n\t"
        "vgatherdps %%xmm2,(%1,%%xmm1,1),%%xmm0\n\tvmovaps %%xmm0,%0"
        : "=m"(dst) : "r"(table), "m"(indices), "m"(mask) : "xmm0", "xmm1", "xmm2", "memory");
    for (int i = 0; i < 4; i++) TEST_ASSERT(dst.f32[i] == table[i], "vgatherdps scale=1 lane %d", i);

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "vgather guard page allocation");
    if (guard != MAP_FAILED) {
        xmm_t zero_mask = { .u64 = {0, 0} };
        xmm_t initial = { .u32 = {0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u} };
        indices.u32[0] = indices.u32[1] = indices.u32[2] = indices.u32[3] = 0;
        __asm__ volatile (
            "vmovaps %4,%%xmm0\n\tvmovdqa %2,%%xmm1\n\tvmovdqa %3,%%xmm2\n\t"
            "vgatherdps %%xmm2,(%1,%%xmm1,4),%%xmm0\n\tvmovaps %%xmm0,%0"
            : "=m"(dst) : "r"(guard), "m"(indices), "m"(zero_mask), "m"(initial)
            : "xmm0", "xmm1", "xmm2", "memory");
        TEST_ASSERT(memcmp(&dst, &initial, sizeof(dst)) == 0, "vgatherdps empty mask suppresses invalid-address fault");
        munmap(guard, (size_t)page_size);
    }
}

int main(void) {
    TEST_START("VGATHERDPS/VGATHERDPD/VGATHERQPS/VGATHERQPD instructions (AVX2)");
    test_vgatherdps_128();
    test_vgatherdps_partial_mask();
    test_vgatherdpd_128();
    test_vgatherqps_128();
    test_vgatherqpd_128();
    test_vgather_boundaries();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
