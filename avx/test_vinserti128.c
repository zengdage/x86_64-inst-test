/* TARGET: VINSERTI128 */
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(7), "c"(0));
    return (ebx & (1U << 5)) != 0;
}
#else
#define check_avx2() 1
#endif

int main(void) {
    TEST_START("VINSERTI128");
    if (!check_avx2()) return 0;

    ymm_t base, out;
    xmm_t inserted;
    for (int i = 0; i < 8; i++) base.u32[i] = UINT32_C(0x1000) + (uint32_t)i;
    for (int i = 0; i < 4; i++) inserted.u32[i] = UINT32_C(0xdead0000) + (uint32_t)i;

    __asm__ volatile("vmovdqa %1, %%ymm0\n\tvmovdqa %2, %%xmm1\n\t"
                     "vinserti128 $0, %%xmm1, %%ymm0, %%ymm2\n\t"
                     "vmovdqa %%ymm2, %0"
                     : "=m"(out) : "m"(base), "m"(inserted)
                     : "ymm0", "xmm1", "ymm2");
    for (int i = 0; i < 8; i++) {
        uint32_t expected = i < 4 ? inserted.u32[i] : base.u32[i];
        TEST_ASSERT(out.u32[i] == expected, "VINSERTI128 low lane %d", i);
    }

    __asm__ volatile("vmovdqa %1, %%ymm0\n\tvmovdqa %2, %%xmm1\n\t"
                     "vinserti128 $1, %%xmm1, %%ymm0, %%ymm2\n\t"
                     "vmovdqa %%ymm2, %0"
                     : "=m"(out) : "m"(base), "m"(inserted)
                     : "ymm0", "xmm1", "ymm2");
    for (int i = 0; i < 8; i++) {
        uint32_t expected = i < 4 ? base.u32[i] : inserted.u32[i - 4];
        TEST_ASSERT(out.u32[i] == expected, "VINSERTI128 high lane %d", i);
    }
    TEST_END();
}
