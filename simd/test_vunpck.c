#include "../common.h"
#include <immintrin.h>

int main(void) {
    TEST_START("VUNPCKHPD/VUNPCKHPS/VUNPCKLPD/VUNPCKLPS 256-bit");

    /* VUNPCKLPD ymm: interleave low doubles from each 128-bit lane
       lane0: [a0,b0], lane1: [a2,b2] */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 4; i++) { a.f64[i] = i + 1.0; b.f64[i] = (i + 1) * 10.0; }
        /* a=[1,2,3,4] b=[10,20,30,40] */
        __asm__ volatile(
            "vmovapd %1, %%ymm0\n\t"
            "vmovapd %2, %%ymm1\n\t"
            "vunpcklpd %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovapd %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        /* lane0: a[0],b[0]; lane1: a[2],b[2] */
        TEST_ASSERT(r.f64[0] == 1.0 && r.f64[1] == 10.0 &&
                    r.f64[2] == 3.0 && r.f64[3] == 30.0,
                    "vunpcklpd: [%g,%g,%g,%g]", r.f64[0],r.f64[1],r.f64[2],r.f64[3]);
    }

    /* VUNPCKHPD ymm: interleave high doubles from each 128-bit lane
       lane0: [a1,b1], lane1: [a3,b3] */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 4; i++) { a.f64[i] = i + 1.0; b.f64[i] = (i + 1) * 10.0; }
        __asm__ volatile(
            "vmovapd %1, %%ymm0\n\t"
            "vmovapd %2, %%ymm1\n\t"
            "vunpckhpd %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovapd %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        TEST_ASSERT(r.f64[0] == 2.0 && r.f64[1] == 20.0 &&
                    r.f64[2] == 4.0 && r.f64[3] == 40.0,
                    "vunpckhpd: [%g,%g,%g,%g]", r.f64[0],r.f64[1],r.f64[2],r.f64[3]);
    }

    /* VUNPCKLPS ymm: interleave low floats from each 128-bit lane
       lane0: [a0,b0,a1,b1], lane1: [a4,b4,a5,b5] */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 8; i++) { a.f32[i] = i + 1.0f; b.f32[i] = (i + 1) * 10.0f; }
        __asm__ volatile(
            "vmovaps %1, %%ymm0\n\t"
            "vmovaps %2, %%ymm1\n\t"
            "vunpcklps %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovaps %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        TEST_ASSERT(r.f32[0] == 1.0f && r.f32[1] == 10.0f &&
                    r.f32[2] == 2.0f && r.f32[3] == 20.0f &&
                    r.f32[4] == 5.0f && r.f32[5] == 50.0f &&
                    r.f32[6] == 6.0f && r.f32[7] == 60.0f,
                    "vunpcklps: [%g,%g,%g,%g,%g,%g,%g,%g]",
                    r.f32[0],r.f32[1],r.f32[2],r.f32[3],
                    r.f32[4],r.f32[5],r.f32[6],r.f32[7]);
    }

    /* VUNPCKHPS ymm: interleave high floats from each 128-bit lane
       lane0: [a2,b2,a3,b3], lane1: [a6,b6,a7,b7] */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 8; i++) { a.f32[i] = i + 1.0f; b.f32[i] = (i + 1) * 10.0f; }
        __asm__ volatile(
            "vmovaps %1, %%ymm0\n\t"
            "vmovaps %2, %%ymm1\n\t"
            "vunpckhps %%ymm1, %%ymm0, %%ymm2\n\t"
            "vmovaps %%ymm2, %0"
            : "=m"(r) : "m"(a), "m"(b) : "ymm0","ymm1","ymm2");
        TEST_ASSERT(r.f32[0] == 3.0f && r.f32[1] == 30.0f &&
                    r.f32[2] == 4.0f && r.f32[3] == 40.0f &&
                    r.f32[4] == 7.0f && r.f32[5] == 70.0f &&
                    r.f32[6] == 8.0f && r.f32[7] == 80.0f,
                    "vunpckhps: [%g,%g,%g,%g,%g,%g,%g,%g]",
                    r.f32[0],r.f32[1],r.f32[2],r.f32[3],
                    r.f32[4],r.f32[5],r.f32[6],r.f32[7]);
    }

    TEST_END();
}
