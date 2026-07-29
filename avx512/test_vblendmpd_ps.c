#include "../common.h"
#include <immintrin.h>

int main(void) {
    TEST_START("VBLENDMPD/VBLENDMPS AVX-512VL");

    /* VBLENDMPD xmm (128-bit): k1 mask selects src2 lanes where bit=1 */
    {
        xmm_t a, b, r;
        a.f64[0] = 1.0; a.f64[1] = 2.0;
        b.f64[0] = 10.0; b.f64[1] = 20.0;
        /* k1 = 0b01 => lane0 from b, lane1 from a */
        __asm__ volatile(
            "vmovapd %1, %%xmm0\n\t"
            "vmovapd %2, %%xmm1\n\t"
            "kmovb %3, %%k1\n\t"
            "vblendmpd %%xmm1, %%xmm0, %%xmm2%{%%k1%}%{z%}\n\t"
            "vmovapd %%xmm2, %0"
            : "=m"(r)
            : "m"(a), "m"(b), "r"((int)0x1)
            : "xmm0","xmm1","xmm2","k1");
        TEST_ASSERT(r.f64[0] == 10.0 && r.f64[1] == 0.0,
                    "vblendmpd xmm k=01: [%g,%g]", r.f64[0], r.f64[1]);
    }

    /* VBLENDMPD xmm: k1 = 0b11 => both lanes from b */
    {
        xmm_t a, b, r;
        a.f64[0] = 1.0; a.f64[1] = 2.0;
        b.f64[0] = 10.0; b.f64[1] = 20.0;
        __asm__ volatile(
            "vmovapd %1, %%xmm0\n\t"
            "vmovapd %2, %%xmm1\n\t"
            "kmovb %3, %%k1\n\t"
            "vblendmpd %%xmm1, %%xmm0, %%xmm2%{%%k1%}%{z%}\n\t"
            "vmovapd %%xmm2, %0"
            : "=m"(r)
            : "m"(a), "m"(b), "r"((int)0x3)
            : "xmm0","xmm1","xmm2","k1");
        TEST_ASSERT(r.f64[0] == 10.0 && r.f64[1] == 20.0,
                    "vblendmpd xmm k=11: [%g,%g]", r.f64[0], r.f64[1]);
    }

    /* VBLENDMPD ymm (256-bit): k1 = 0b1010 => lanes 1,3 from b */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 4; i++) { a.f64[i] = i + 1.0; b.f64[i] = (i + 1) * 10.0; }
        __asm__ volatile(
            "vmovapd %1, %%ymm0\n\t"
            "vmovapd %2, %%ymm1\n\t"
            "kmovb %3, %%k1\n\t"
            "vblendmpd %%ymm1, %%ymm0, %%ymm2%{%%k1%}%{z%}\n\t"
            "vmovapd %%ymm2, %0"
            : "=m"(r)
            : "m"(a), "m"(b), "r"((int)0xA)
            : "ymm0","ymm1","ymm2","k1");
        /* k=1010: bit0=0->zero, bit1=1->b[1]=20, bit2=0->zero, bit3=1->b[3]=40 */
        TEST_ASSERT(r.f64[0] == 0.0  && r.f64[1] == 20.0 &&
                    r.f64[2] == 0.0  && r.f64[3] == 40.0,
                    "vblendmpd ymm k=1010: [%g,%g,%g,%g]",
                    r.f64[0],r.f64[1],r.f64[2],r.f64[3]);
    }

    /* VBLENDMPS xmm (128-bit): k1 = 0b0101 => lanes 0,2 from b */
    {
        xmm_t a, b, r;
        for (int i = 0; i < 4; i++) { a.f32[i] = i + 1.0f; b.f32[i] = (i + 1) * 10.0f; }
        __asm__ volatile(
            "vmovaps %1, %%xmm0\n\t"
            "vmovaps %2, %%xmm1\n\t"
            "kmovb %3, %%k1\n\t"
            "vblendmps %%xmm1, %%xmm0, %%xmm2%{%%k1%}%{z%}\n\t"
            "vmovaps %%xmm2, %0"
            : "=m"(r)
            : "m"(a), "m"(b), "r"((int)0x5)
            : "xmm0","xmm1","xmm2","k1");
        TEST_ASSERT(r.f32[0] == 10.0f && r.f32[1] == 0.0f &&
                    r.f32[2] == 30.0f && r.f32[3] == 0.0f,
                    "vblendmps xmm k=0101: [%g,%g,%g,%g]",
                    r.f32[0],r.f32[1],r.f32[2],r.f32[3]);
    }

    /* VBLENDMPS ymm (256-bit): k1 = 0xFF => all lanes from b */
    {
        ymm_t a, b, r;
        for (int i = 0; i < 8; i++) { a.f32[i] = i + 1.0f; b.f32[i] = (i + 1) * 10.0f; }
        __asm__ volatile(
            "vmovaps %1, %%ymm0\n\t"
            "vmovaps %2, %%ymm1\n\t"
            "kmovb %3, %%k1\n\t"
            "vblendmps %%ymm1, %%ymm0, %%ymm2%{%%k1%}%{z%}\n\t"
            "vmovaps %%ymm2, %0"
            : "=m"(r)
            : "m"(a), "m"(b), "r"((int)0xFF)
            : "ymm0","ymm1","ymm2","k1");
        int ok = 1;
        for (int i = 0; i < 8; i++) if (r.f32[i] != (i + 1) * 10.0f) ok = 0;
        TEST_ASSERT(ok, "vblendmps ymm k=FF: all from b");
    }

    TEST_END();
}
