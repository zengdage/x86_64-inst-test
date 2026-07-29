/*
 * test_vshufps_pd.c - Test VSHUFPS/VSHUFPD (256-bit AVX)
 *
 * AVX 256-bit shuffle operates on two 128-bit lanes independently.
 * VSHUFPS: Each 128-bit lane shuffles like SSE SHUFPS.
 * VSHUFPD: Each 128-bit lane shuffles like SSE SHUFPD.
 *
 * Compile: gcc -o test_vshufps_pd simd/test_vshufps_pd.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vshufps_256(void) {
    ymm_t a = { .f32 = {1.0f,2.0f,3.0f,4.0f, 5.0f,6.0f,7.0f,8.0f} };
    ymm_t b = { .f32 = {10.0f,20.0f,30.0f,40.0f, 50.0f,60.0f,70.0f,80.0f} };
    ymm_t dst;

    /* 0x00: all select element 0 from each source half */
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vshufps $0x00, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    /* Low lane: a[0],a[0],b[0],b[0] */
    TEST_ASSERT(dst.f32[0] == 1.0f, "vshufps $00 low [0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[1] == 1.0f, "vshufps $00 low [1]: got %f", dst.f32[1]);
    TEST_ASSERT(dst.f32[2] == 10.0f, "vshufps $00 low [2]: got %f", dst.f32[2]);
    TEST_ASSERT(dst.f32[3] == 10.0f, "vshufps $00 low [3]: got %f", dst.f32[3]);
    /* High lane: a[4],a[4],b[4],b[4] */
    TEST_ASSERT(dst.f32[4] == 5.0f, "vshufps $00 high [4]: got %f", dst.f32[4]);
    TEST_ASSERT(dst.f32[5] == 5.0f, "vshufps $00 high [5]: got %f", dst.f32[5]);
    TEST_ASSERT(dst.f32[6] == 50.0f, "vshufps $00 high [6]: got %f", dst.f32[6]);
    TEST_ASSERT(dst.f32[7] == 50.0f, "vshufps $00 high [7]: got %f", dst.f32[7]);
}

static void test_vshufps_identity(void) {
    ymm_t a = { .f32 = {1.0f,2.0f,3.0f,4.0f, 5.0f,6.0f,7.0f,8.0f} };
    ymm_t b = { .f32 = {10.0f,20.0f,30.0f,40.0f, 50.0f,60.0f,70.0f,80.0f} };
    ymm_t dst;

    /* 0xE4 = identity within each lane */
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vshufps $0xE4, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f && dst.f32[1] == 2.0f,
        "vshufps $E4 low: a[0],a[1]");
    TEST_ASSERT(dst.f32[2] == 30.0f && dst.f32[3] == 40.0f,
        "vshufps $E4 low: b[2],b[3]");
}

static void test_vshufpd_256(void) {
    ymm_t a = { .f64 = {1.0, 2.0, 3.0, 4.0} };
    ymm_t b = { .f64 = {5.0, 6.0, 7.0, 8.0} };
    ymm_t dst;

    /* imm8=0x5 = 0101: bit0=1(a[1]),bit1=0(b[0]),bit2=1(a[3]),bit3=0(b[2]) */
    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vshufpd $0x5, %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.f64[0] == 2.0, "vshufpd [0]: got %f", dst.f64[0]);
    TEST_ASSERT(dst.f64[1] == 5.0, "vshufpd [1]: got %f", dst.f64[1]);
    TEST_ASSERT(dst.f64[2] == 4.0, "vshufpd [2]: got %f", dst.f64[2]);
    TEST_ASSERT(dst.f64[3] == 7.0, "vshufpd [3]: got %f", dst.f64[3]);
}

int main(void) {
    TEST_START("VSHUFPS/VSHUFPD instructions (AVX 256-bit)");
    test_vshufps_256();
    test_vshufps_identity();
    test_vshufpd_256();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
