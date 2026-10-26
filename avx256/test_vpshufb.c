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
    TEST_START("VPSHUFB");
    if (!check_avx2()) { printf("AVX2 not supported, skipping\n"); return 0; }
    xmm_t xs, xm, xo;
    for (int i = 0; i < 16; i++) {
        xs.u8[i] = (uint8_t)(0xa0 + i);
        xm.u8[i] = (uint8_t)(i == 1 ? 0x80 : (i == 2 ? 0x0f : 15 - i));
    }
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\tvpshufb %%xmm1, %%xmm0, %%xmm2\n\tvmovdqu %%xmm2, %0"
                     : "=m"(xo) : "m"(xs), "m"(xm) : "xmm0", "xmm1", "xmm2");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(xo.u8[i] == ((xm.u8[i] & 0x80) ? 0 : xs.u8[xm.u8[i] & 0x0f]), "vpshufb xmm lane %d", i);

    ymm_t ys, ym, yo;
    for (int i = 0; i < 32; i++) {
        ys.u8[i] = (uint8_t)(0x40 + i);
        ym.u8[i] = (uint8_t)((i % 16 == 0) ? 0x0f : (i % 16 == 1 ? 0x80 : i % 16));
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\tvpshufb %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(yo) : "m"(ys), "m"(ym) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 32; i++) {
        int base = (i / 16) * 16;
        uint8_t expected = (ym.u8[i] & 0x80) ? 0 : ys.u8[base + (ym.u8[i] & 0x0f)];
        TEST_ASSERT(yo.u8[i] == expected, "vpshufb ymm lane %d", i);
    }
    __asm__ volatile("vmovdqu %0, %%ymm0\n\tvmovdqu %1, %%ymm1\n\tvpshufb %%ymm1, %%ymm0, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "+m"(ys) : "m"(ym) : "ymm0", "ymm1");
    for (int i = 0; i < 32; i++) {
        int base = (i / 16) * 16;
        uint8_t expected = (ym.u8[i] & 0x80) ? 0 : (uint8_t)(0x40 + base + (ym.u8[i] & 0x0f));
        TEST_ASSERT(ys.u8[i] == expected, "vpshufb aliased lane %d", i);
    }
    __asm__ volatile("vzeroupper");
    TEST_END();
}
