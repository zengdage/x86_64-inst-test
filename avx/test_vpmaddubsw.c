/* TARGET: VPMADDUBSW */
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

static int16_t sat16(int32_t value) {
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

int main(void) {
    TEST_START("VPMADDUBSW");
    if (!check_avx2()) { printf("AVX2 not supported, skipping\n"); return 0; }
    ymm_t a, b, out;
    for (int i = 0; i < 32; i++) { a.u8[i] = (uint8_t)(i * 17); b.i8[i] = (int8_t)((i & 1) ? -127 : 127); }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\tvpmaddubsw %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 16; i++) {
        int32_t value = (int32_t)a.u8[2 * i] * b.i8[2 * i] + (int32_t)a.u8[2 * i + 1] * b.i8[2 * i + 1];
        TEST_ASSERT(out.i16[i] == sat16(value), "vpmaddubsw ymm lane %d", i);
    }
    xmm_t ax = { .u8 = {255, 127, 2, 3, 128, 2, 0, 0, 255, 255, 1, 1, 0x80, 0x80, 0x7f, 0x7f} };
    xmm_t bx = { .i8 = {-128, 127, 100, -100, -128, -128, 1, -1, 127, 127, 127, 127, -128, 127, 127, -128} };
    xmm_t ox;
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\tvpmaddubsw %%xmm1, %%xmm0, %%xmm2\n\tvmovdqu %%xmm2, %0"
                     : "=m"(ox) : "m"(ax), "m"(bx) : "xmm0", "xmm1", "xmm2");
    for (int i = 0; i < 8; i++) {
        int32_t value = (int32_t)ax.u8[2 * i] * bx.i8[2 * i] + (int32_t)ax.u8[2 * i + 1] * bx.i8[2 * i + 1];
        TEST_ASSERT(ox.i16[i] == sat16(value), "vpmaddubsw xmm lane %d", i);
    }
    __asm__ volatile("vzeroupper");
    TEST_END();
}
