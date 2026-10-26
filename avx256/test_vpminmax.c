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
    TEST_START("VPMINSD/VPMAXSD/VPMAXUB");
    if (!check_avx2()) { printf("AVX2 not supported, skipping\n"); return 0; }
    ymm_t a = { .i32 = {INT32_MIN, -1, 0, INT32_MAX, 7, -7, 100, -100} };
    ymm_t b = { .i32 = {INT32_MAX, -2, 1, INT32_MIN, -7, 7, -100, 100} };
    ymm_t out;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\tvpminsd %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++) TEST_ASSERT(out.i32[i] == (a.i32[i] < b.i32[i] ? a.i32[i] : b.i32[i]), "vpminsd lane %d", i);
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\tvpmaxsd %%xmm1, %%xmm0, %%xmm2\n\tvmovdqu %%xmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2");
    for (int i = 0; i < 4; i++) TEST_ASSERT(out.i32[i] == (a.i32[i] > b.i32[i] ? a.i32[i] : b.i32[i]), "vpmaxsd VEX.128 lane %d", i);

    ymm_t ua = { .u8 = {0, 1, 0xff, 128, 200, 3, 250, 7, 9, 255, 11, 12, 13, 14, 15, 16,
                        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32} };
    ymm_t ub = { .u8 = {1, 0, 1, 127, 100, 4, 249, 8, 10, 254, 12, 11, 14, 13, 16, 15,
                        16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30, 33} };
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\tvpmaxub %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(ua), "m"(ub) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 32; i++) TEST_ASSERT(out.u8[i] == (ua.u8[i] > ub.u8[i] ? ua.u8[i] : ub.u8[i]), "vpmaxub lane %d", i);
    __asm__ volatile("vzeroupper");
    TEST_END();
}
