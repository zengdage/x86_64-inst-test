#include "../common.h"
#include <immintrin.h>

static void test_vunpck_128_all_forms(void) {
    xmm_t a32 = { .u32 = {
        UINT32_C(0x80000000), UINT32_C(0x7fc12345),
        UINT32_C(0x00000001), UINT32_C(0xff800000)
    } };
    xmm_t b32 = { .u32 = {
        UINT32_C(0x3f800000), UINT32_C(0xdeadbeef),
        UINT32_C(0x40000000), UINT32_C(0x7f800001)
    } };
    xmm_t a64 = { .u64 = {
        UINT64_C(0x8000000000000000), UINT64_C(0x7ff8123456789abc)
    } };
    xmm_t b64 = { .u64 = {
        UINT64_C(0x0000000000000001), UINT64_C(0xfff0000000000000)
    } };
    ymm_t initial, result;
    memset(&initial, 0xa5, sizeof(initial));

#define TEST_VUNPCK128(INSN, A, B, EXPECTED, LABEL) do {                    \
        xmm_t expected = (EXPECTED);                                        \
        __asm__ volatile (                                                  \
            "vmovdqu %1, %%ymm2\n\t" "vmovdqu %2, %%xmm0\n\t"         \
            INSN " %3, %%xmm0, %%xmm2\n\t" "vmovdqu %%ymm2, %0"         \
            : "=m"(result) : "m"(initial), "m"(A), "m"(B)              \
            : "xmm0", "ymm2");                                          \
        TEST_ASSERT(memcmp(&result, &expected, sizeof(expected)) == 0,       \
                    LABEL " exact low 128-bit interleave");                 \
        TEST_ASSERT(result.u64[2] == 0 && result.u64[3] == 0,                \
                    LABEL " VEX.128 clears YMM[255:128]");                  \
    } while (0)

    TEST_VUNPCK128("vunpcklps", a32, b32,
                   ((xmm_t){ .u32 = {a32.u32[0], b32.u32[0], a32.u32[1], b32.u32[1]} }),
                   "VUNPCKLPS");
    TEST_VUNPCK128("vunpckhps", a32, b32,
                   ((xmm_t){ .u32 = {a32.u32[2], b32.u32[2], a32.u32[3], b32.u32[3]} }),
                   "VUNPCKHPS");
    TEST_VUNPCK128("vunpcklpd", a64, b64,
                   ((xmm_t){ .u64 = {a64.u64[0], b64.u64[0]} }),
                   "VUNPCKLPD");
    TEST_VUNPCK128("vunpckhpd", a64, b64,
                   ((xmm_t){ .u64 = {a64.u64[1], b64.u64[1]} }),
                   "VUNPCKHPD");
#undef TEST_VUNPCK128
}

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

    /* VEX.128 form is lane-local and zeroes the upper half of the YMM destination. */
    {
        xmm_t a = { .u32 = {1,2,3,4} };
        xmm_t b = { .u32 = {10,20,30,40} };
        ymm_t initial, result;
        memset(&initial, 0xa5, sizeof(initial));
        __asm__ volatile (
            "vmovdqu %1, %%ymm2\n\t" "vmovdqu %2, %%xmm0\n\t"
            "vunpcklps %3, %%xmm0, %%xmm2\n\t" "vmovdqu %%ymm2, %0"
            : "=m"(result) : "m"(initial), "m"(a), "m"(b)
            : "xmm0", "ymm2");
        uint32_t expected[] = {1,10,2,20};
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(result.u32[i] == expected[i], "vunpcklps xmm memory source lane %d", i);
        for (int i = 4; i < 8; i++)
            TEST_ASSERT(result.u32[i] == 0, "VEX.128 vunpcklps zeroes upper ymm lane %d", i);
    }

    test_vunpck_128_all_forms();

    TEST_END();
}
