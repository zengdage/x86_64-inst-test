#include "../common.h"
#include <immintrin.h>

int main(void) {
    TEST_START("VEXTRACTF32X4/VEXTRACTF64X2/VEXTRACTI32X4/VEXTRACTI64X2 AVX-512");

    /* VEXTRACTF32X4: extract 128-bit lane of floats from 512-bit zmm */
    /* imm8=0 => lane0, imm8=1 => lane1, imm8=2 => lane2, imm8=3 => lane3 */
    {
        float src[16] __attribute__((aligned(64)));
        for (int i = 0; i < 16; i++) src[i] = (float)(i + 1);
        xmm_t r0, r1, r2, r3;
        __asm__ volatile(
            "vmovaps %4, %%zmm0\n\t"
            "vextractf32x4 $0, %%zmm0, %0\n\t"
            "vextractf32x4 $1, %%zmm0, %1\n\t"
            "vextractf32x4 $2, %%zmm0, %2\n\t"
            "vextractf32x4 $3, %%zmm0, %3"
            : "=m"(r0), "=m"(r1), "=m"(r2), "=m"(r3)
            : "m"(src[0])
            : "zmm0");
        TEST_ASSERT(r0.f32[0]==1 && r0.f32[3]==4,  "vextractf32x4 lane0: [%g..%g]", r0.f32[0], r0.f32[3]);
        TEST_ASSERT(r1.f32[0]==5 && r1.f32[3]==8,  "vextractf32x4 lane1: [%g..%g]", r1.f32[0], r1.f32[3]);
        TEST_ASSERT(r2.f32[0]==9 && r2.f32[3]==12, "vextractf32x4 lane2: [%g..%g]", r2.f32[0], r2.f32[3]);
        TEST_ASSERT(r3.f32[0]==13&& r3.f32[3]==16, "vextractf32x4 lane3: [%g..%g]", r3.f32[0], r3.f32[3]);
    }

    /* VEXTRACTF64X2: extract 128-bit lane of doubles from 512-bit zmm */
    {
        double src[8] __attribute__((aligned(64)));
        for (int i = 0; i < 8; i++) src[i] = (double)(i + 1);
        xmm_t r0, r1, r2, r3;
        __asm__ volatile(
            "vmovapd %4, %%zmm0\n\t"
            "vextractf64x2 $0, %%zmm0, %0\n\t"
            "vextractf64x2 $1, %%zmm0, %1\n\t"
            "vextractf64x2 $2, %%zmm0, %2\n\t"
            "vextractf64x2 $3, %%zmm0, %3"
            : "=m"(r0), "=m"(r1), "=m"(r2), "=m"(r3)
            : "m"(src[0])
            : "zmm0");
        TEST_ASSERT(r0.f64[0]==1 && r0.f64[1]==2, "vextractf64x2 lane0: [%g,%g]", r0.f64[0], r0.f64[1]);
        TEST_ASSERT(r1.f64[0]==3 && r1.f64[1]==4, "vextractf64x2 lane1: [%g,%g]", r1.f64[0], r1.f64[1]);
        TEST_ASSERT(r2.f64[0]==5 && r2.f64[1]==6, "vextractf64x2 lane2: [%g,%g]", r2.f64[0], r2.f64[1]);
        TEST_ASSERT(r3.f64[0]==7 && r3.f64[1]==8, "vextractf64x2 lane3: [%g,%g]", r3.f64[0], r3.f64[1]);
    }

    /* VEXTRACTI32X4: extract 128-bit lane of int32 from 512-bit zmm */
    {
        int32_t src[16] __attribute__((aligned(64)));
        for (int i = 0; i < 16; i++) src[i] = i + 1;
        xmm_t r0, r2;
        __asm__ volatile(
            "vmovdqu32 %2, %%zmm0\n\t"
            "vextracti32x4 $0, %%zmm0, %0\n\t"
            "vextracti32x4 $2, %%zmm0, %1"
            : "=m"(r0), "=m"(r2)
            : "m"(src[0])
            : "zmm0");
        TEST_ASSERT(r0.i32[0]==1 && r0.i32[3]==4,  "vextracti32x4 lane0: [%d..%d]", r0.i32[0], r0.i32[3]);
        TEST_ASSERT(r2.i32[0]==9 && r2.i32[3]==12, "vextracti32x4 lane2: [%d..%d]", r2.i32[0], r2.i32[3]);
    }

    /* VEXTRACTI64X2: extract 128-bit lane of int64 from 512-bit zmm */
    {
        int64_t src[8] __attribute__((aligned(64)));
        for (int i = 0; i < 8; i++) src[i] = i + 1;
        xmm_t r1, r3;
        __asm__ volatile(
            "vmovdqu64 %2, %%zmm0\n\t"
            "vextracti64x2 $1, %%zmm0, %0\n\t"
            "vextracti64x2 $3, %%zmm0, %1"
            : "=m"(r1), "=m"(r3)
            : "m"(src[0])
            : "zmm0");
        TEST_ASSERT(r1.i64[0]==3 && r1.i64[1]==4, "vextracti64x2 lane1: [%ld,%ld]", r1.i64[0], r1.i64[1]);
        TEST_ASSERT(r3.i64[0]==7 && r3.i64[1]==8, "vextracti64x2 lane3: [%ld,%ld]", r3.i64[0], r3.i64[1]);
    }

    /* 256-bit forms: VEXTRACTF32X4 from ymm (imm8=0 or 1) */
    {
        float src[8] __attribute__((aligned(32)));
        for (int i = 0; i < 8; i++) src[i] = (float)(i + 1);
        xmm_t r0, r1;
        __asm__ volatile(
            "vmovaps %2, %%ymm0\n\t"
            "vextractf32x4 $0, %%ymm0, %0\n\t"
            "vextractf32x4 $1, %%ymm0, %1"
            : "=m"(r0), "=m"(r1)
            : "m"(src[0])
            : "ymm0");
        TEST_ASSERT(r0.f32[0]==1 && r0.f32[3]==4, "vextractf32x4 ymm lane0: [%g..%g]", r0.f32[0], r0.f32[3]);
        TEST_ASSERT(r1.f32[0]==5 && r1.f32[3]==8, "vextractf32x4 ymm lane1: [%g..%g]", r1.f32[0], r1.f32[3]);
    }

    TEST_END();
}
