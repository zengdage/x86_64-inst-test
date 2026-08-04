#include "../common.h"
#include <immintrin.h>
#include <math.h>
#include <float.h>

int main(void) {
    TEST_START("VSCALEFPD/VSCALEFPS/VSCALEFSD/VSCALEFSS AVX-512");

    /* VSCALEFSS: dst = src1 * 2^floor(src2) */
    {
        xmm_t a, b, r;
        memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b)); memset(&r, 0, sizeof(r));
        a.f32[0] = 1.5f;
        b.f32[0] = 3.0f; /* 2^3 = 8 => 1.5 * 8 = 12 */
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "vscalefss %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2");
        TEST_ASSERT(r.f32[0] == 12.0f, "vscalefss 1.5*2^3=%g", r.f32[0]);
    }

    /* VSCALEFSS: negative scale => division */
    {
        xmm_t a, b, r;
        memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b)); memset(&r, 0, sizeof(r));
        a.f32[0] = 16.0f;
        b.f32[0] = -2.0f; /* 2^(-2) = 0.25 => 16 * 0.25 = 4 */
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "vscalefss %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2");
        TEST_ASSERT(r.f32[0] == 4.0f, "vscalefss 16*2^-2=%g", r.f32[0]);
    }

    /* VSCALEFSS: fractional scale => floor */
    {
        xmm_t a, b, r;
        memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b)); memset(&r, 0, sizeof(r));
        a.f32[0] = 1.0f;
        b.f32[0] = 2.7f; /* floor(2.7)=2 => 2^2=4 => 1.0*4=4.0 */
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "vscalefss %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2");
        TEST_ASSERT(r.f32[0] == 4.0f, "vscalefss 1.0*2^floor(2.7)=%g", r.f32[0]);
    }

    /* VSCALEFSD: double precision scalar */
    {
        xmm_t a, b, r;
        memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b)); memset(&r, 0, sizeof(r));
        a.f64[0] = 2.5;
        b.f64[0] = 4.0; /* 2^4=16 => 2.5*16=40 */
        __asm__ volatile(
            "vmovapd %1, %%xmm0\n\t"
            "vmovapd %2, %%xmm1\n\t"
            "vscalefsd %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovapd %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2");
        TEST_ASSERT(r.f64[0] == 40.0, "vscalefsd 2.5*2^4=%g", r.f64[0]);
    }

    /* VSCALEFPS xmm: packed float 128-bit */
    {
        xmm_t a, b, r;
        a.f32[0] = 1.0f; a.f32[1] = 2.0f; a.f32[2] = 3.0f; a.f32[3] = 4.0f;
        b.f32[0] = 0.0f; b.f32[1] = 1.0f; b.f32[2] = 2.0f; b.f32[3] = 3.0f;
        /* results: 1*1=1, 2*2=4, 3*4=12, 4*8=32 */
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "vscalefps %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2");
        TEST_ASSERT(r.f32[0]==1.0f && r.f32[1]==4.0f && r.f32[2]==12.0f && r.f32[3]==32.0f,
                    "vscalefps xmm: [%g,%g,%g,%g]", r.f32[0],r.f32[1],r.f32[2],r.f32[3]);
    }

    /* VSCALEFPD ymm: packed double 256-bit */
    {
        ymm_t a, b, r;
        a.f64[0] = 1.0; a.f64[1] = 2.0; a.f64[2] = 0.5; a.f64[3] = 10.0;
        b.f64[0] = 1.0; b.f64[1] = 2.0; b.f64[2] = 3.0; b.f64[3] = -1.0;
        /* results: 1*2=2, 2*4=8, 0.5*8=4, 10*0.5=5 */
        __asm__ volatile(
            "vmovapd %1, %%ymm0\n\t"
            "vmovapd %2, %%ymm1\n\t"
            "vscalefpd %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovapd %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        TEST_ASSERT(r.f64[0]==2.0 && r.f64[1]==8.0 && r.f64[2]==4.0 && r.f64[3]==5.0,
                    "vscalefpd ymm: [%g,%g,%g,%g]", r.f64[0],r.f64[1],r.f64[2],r.f64[3]);
    }

    /* VSCALEFSS with scale=0 => multiply by 1 */
    {
        xmm_t a, b, r;
        memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b)); memset(&r, 0, sizeof(r));
        a.f32[0] = 42.0f;
        b.f32[0] = 0.0f;
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "vscalefss %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0","xmm1","xmm2");
        TEST_ASSERT(r.f32[0] == 42.0f, "vscalefss scale=0: %g", r.f32[0]);
    }

    /* Boundary and IEEE-754 special values. */
    {
        xmm_t a = { .f32 = {1.0f, -0.0f, INFINITY, NAN} };
        xmm_t b = { .f32 = {127.0f, -149.0f, -INFINITY, 0.0f} };
        xmm_t r;
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "vscalefps %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2");
        TEST_ASSERT(r.f32[0] == 0x1p127f, "vscalefps largest power-of-two finite boundary");
        TEST_ASSERT(r.f32[1] == 0.0f && signbit(r.f32[1]),
                    "vscalefps negative zero preserves sign");
        TEST_ASSERT(isnan(r.f32[2]), "vscalefps +inf scaled by -inf is NaN");
        TEST_ASSERT(isnan(r.f32[3]), "vscalefps NaN propagates");
    }

    {
        xmm_t a = { .f32 = {FLT_MAX, 0.0f, 0.0f, 0.0f} };
        xmm_t b = { .f32 = {1.0f, 0.0f, 0.0f, 0.0f} };
        xmm_t r;
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "vscalefss %%xmm1, %%xmm0, %%xmm2\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2");
        TEST_ASSERT(isinf(r.f32[0]) && r.f32[0] > 0.0f,
                    "vscalefss FLT_MAX scaled up overflows to +inf");
    }

    TEST_END();
}
