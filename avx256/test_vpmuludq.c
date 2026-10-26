#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx, xcr0_lo, xcr0_hi;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
    if (!(ecx & (1U << 27)) || !(ecx & (1U << 28))) return 0;
    __asm__ volatile("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
    if ((xcr0_lo & 0x6U) != 0x6U) return 0;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx & (1U << 5)) != 0;
}
#else
#define check_avx2() 1
#endif

int main(void) {
    TEST_START("VPMULUDQ");
    if (!check_avx2()) { printf("AVX2 not supported, skipping\n"); return 0; }
    xmm_t a = { .u32 = {UINT32_MAX, 1, 0x80000000U, 3} };
    xmm_t b = { .u32 = {2, UINT32_MAX, 2, 7} };
    ymm_t out;
    __asm__ volatile("vpcmpeqd %%ymm0, %%ymm0, %%ymm0\n\tvmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\tvpmuludq %%xmm1, %%xmm0, %%xmm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm1");
    TEST_ASSERT(out.u64[0] == (uint64_t)a.u32[0] * b.u32[0], "vpmuludq xmm low pair");
    TEST_ASSERT(out.u64[1] == (uint64_t)a.u32[2] * b.u32[2], "vpmuludq xmm high pair");
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0, "vpmuludq VEX.128 upper zeroing");

    ymm_t ya = { .u32 = {UINT32_MAX, 1, 2, 3, 0x80000000U, 5, 7, 9} };
    ymm_t yb = { .u32 = {2, 4, 5, 6, 2, 8, UINT32_MAX, 10} };
    ymm_t out256;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\tvpmuludq %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out256) : "m"(ya), "m"(yb) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; i++) TEST_ASSERT(out256.u64[i] == (uint64_t)ya.u32[2 * i] * yb.u32[2 * i], "vpmuludq ymm lane %d", i);
    __asm__ volatile("vzeroupper");
    TEST_END();
}
