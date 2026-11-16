/* TARGETS: VPERMILPD VINSERTPS VEXTRACTPS VROUNDSS VROUNDSD */
#include "../common.h"

int main(void) {
    ymm_t out;
    const ymm_t pd = { .f64 = {1.0, 2.0, 3.0, 4.0} };
    const xmm_t a = { .u32 = {10, 11, 12, 13} };
    const xmm_t b = { .u32 = {20, 21, 22, 23} };
    xmm_t inserted;
    uint64_t extracted = UINT64_MAX;

    TEST_START("AVX VEX instruction gaps");
    __asm__ volatile("vpermilpd $5, %1, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(pd) : "ymm0");
    TEST_ASSERT(out.f64[0] == 2.0 && out.f64[1] == 1.0,
                "VPERMILPD low 128-bit lane");
    TEST_ASSERT(out.f64[2] == 4.0 && out.f64[3] == 3.0,
                "VPERMILPD high 128-bit lane");

    __asm__ volatile("vinsertps $0x69, %2, %1, %%xmm0\n\tvmovdqu %%xmm0, %0"
                     : "=m"(inserted) : "x"(a), "x"(b) : "xmm0");
    TEST_ASSERT(inserted.u32[0] == 0 && inserted.u32[1] == a.u32[1],
                "VINSERTPS zero-mask and preserved lanes");
    TEST_ASSERT(inserted.u32[2] == b.u32[1] && inserted.u32[3] == 0,
                "VINSERTPS selected source and zero mask");

    __asm__ volatile("vextractps $0xff, %1, %k0" : "+r"(extracted) : "x"(b));
    TEST_ASSERT(extracted == b.u32[3], "VEXTRACTPS index truncation and GPR zero extension");

    const xmm_t scalar_ps = { .f32 = {2.5f, 20.0f, 30.0f, 40.0f} };
    const xmm_t scalar_pd = { .f64 = {-2.5, 99.0} };
    const xmm_t keep_ps = { .u32 = {0, 0x11111111, 0x22222222, 0x33333333} };
    const xmm_t keep_pd = { .u64 = {0, UINT64_C(0x4444444455555555)} };
    memset(&out, 0xa5, sizeof(out));
    __asm__ volatile("vroundss $0, %2, %1, %%xmm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "x"(keep_ps), "m"(scalar_ps) : "xmm0");
    TEST_ASSERT(out.f32[0] == 2.0f, "VROUNDSS ties-to-even result");
    for (int i = 1; i < 4; i++)
        TEST_ASSERT(out.u32[i] == keep_ps.u32[i], "VROUNDSS preserved lane %d", i);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(out.u32[i] == 0, "VROUNDSS clears upper YMM lane %d", i);

    memset(&out, 0xa5, sizeof(out));
    __asm__ volatile("vroundsd $0, %2, %1, %%xmm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "x"(keep_pd), "m"(scalar_pd) : "xmm0");
    TEST_ASSERT(out.f64[0] == -2.0, "VROUNDSD ties-to-even result");
    TEST_ASSERT(out.u64[1] == keep_pd.u64[1], "VROUNDSD preserved high qword");
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0, "VROUNDSD clears upper YMM");
    TEST_END();
}
