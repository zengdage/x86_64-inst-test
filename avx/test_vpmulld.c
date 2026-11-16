/* TARGET: VPMULLD */
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

static uint32_t mul32(uint32_t a, uint32_t b) { return (uint32_t)((uint64_t)a * b); }

int main(void) {
    TEST_START("VPMULLD");
    if (!check_avx2()) { printf("AVX2 not supported, skipping\n"); return 0; }
    xmm_t a = { .i32 = {INT32_MIN, INT32_MAX, -1, 0x12345678} };
    xmm_t b = { .i32 = {-1, 2, INT32_MIN, -3} };
    ymm_t out;
    __asm__ volatile("vpcmpeqd %%ymm0, %%ymm0, %%ymm0\n\tvmovdqu %1, %%xmm0\n\tvmovdqu %2, %%xmm1\n\tvpmulld %%xmm1, %%xmm0, %%xmm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm1");
    for (int i = 0; i < 4; i++) TEST_ASSERT(out.u32[i] == mul32((uint32_t)a.i32[i], (uint32_t)b.i32[i]), "vpmulld xmm lane %d", i);
    for (int i = 4; i < 8; i++) TEST_ASSERT(out.u32[i] == 0, "vpmulld VEX.128 upper lane %d", i);

    ymm_t ya = { .i32 = {0, -1, INT32_MIN, INT32_MAX, 3, -7, 0x40000000, -0x40000000} };
    ymm_t yb = { .i32 = {-1, -1, 2, 2, INT32_MAX, INT32_MIN, 8, 8} };
    ymm_t ya_before = ya;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\tvpmulld %%ymm1, %%ymm0, %%ymm0\n\tvmovdqu %%ymm0, %0"
                     : "+m"(ya) : "m"(ya), "m"(yb) : "ymm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(ya.u32[i] == mul32((uint32_t)ya_before.i32[i], (uint32_t)yb.i32[i]),
                    "vpmulld ymm alias lane %d", i);
    __asm__ volatile("vzeroupper");
    TEST_END();
}
