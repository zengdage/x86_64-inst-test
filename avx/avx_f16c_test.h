#ifndef X86_64_AVX_F16C_TEST_H
#define X86_64_AVX_F16C_TEST_H

#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_f16c(void) {
    uint32_t eax, ebx, ecx, edx, xcr0_lo, xcr0_hi;
    __asm__("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(1), "c"(0));
    if ((ecx & ((1U << 27) | (1U << 28) | (1U << 29))) !=
        ((1U << 27) | (1U << 28) | (1U << 29)))
        return 0;
    __asm__("xgetbv" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
    return (xcr0_lo & 0x6U) == 0x6U;
}
#else
#define check_f16c() 1
#endif

#if TEST_PS_TO_PH
int main(void) {
    const ymm_t input = { .u32 = {
        UINT32_C(0x00000000), UINT32_C(0x80000000),
        UINT32_C(0x3f800000), UINT32_C(0xc0000000),
        UINT32_C(0x477fe000), UINT32_C(0x38800000),
        UINT32_C(0x33800000), UINT32_C(0x7f800000)
    } };
    const uint16_t expected[8] = {
        0x0000, 0x8000, 0x3c00, 0xc000, 0x7bff, 0x0400, 0x0001, 0x7c00
    };
    xmm_t out;
    TEST_START("VCVTPS2PH VEX");
    if (!check_f16c()) return 0;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     "vcvtps2ph $0, %%ymm0, %%xmm1\n\t"
                     "vmovdqu %%xmm1, %0"
                     : "=m"(out) : "m"(input) : "ymm0", "xmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u16[i] == expected[i],
                    "VCVTPS2PH ymm lane %d: %#x", i, out.u16[i]);

    memset(&out, 0xa5, sizeof(out));
    __asm__ volatile("movdqu %1, %%xmm0\n\t"
                     "vcvtps2ph $0, %%xmm0, %%xmm1\n\t"
                     "vmovdqu %%xmm1, %0"
                     : "=m"(out) : "m"(input) : "xmm0", "xmm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u16[i] == expected[i],
                    "VCVTPS2PH xmm lane %d", i);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(out.u16[i] == 0, "VCVTPS2PH xmm zero lane %d", i);
    TEST_END();
}
#else
int main(void) {
    const xmm_t input = { .u16 = {
        0x0000, 0x8000, 0x3c00, 0xc000, 0x7bff, 0x0400, 0x0001, 0x7c00
    } };
    const uint32_t expected[8] = {
        UINT32_C(0x00000000), UINT32_C(0x80000000),
        UINT32_C(0x3f800000), UINT32_C(0xc0000000),
        UINT32_C(0x477fe000), UINT32_C(0x38800000),
        UINT32_C(0x33800000), UINT32_C(0x7f800000)
    };
    ymm_t out, initial;
    TEST_START("VCVTPH2PS VEX");
    if (!check_f16c()) return 0;
    __asm__ volatile("vmovdqu %1, %%xmm0\n\tvcvtph2ps %%xmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(input) : "xmm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == expected[i],
                    "VCVTPH2PS ymm lane %d: %#x", i, out.u32[i]);

    __asm__ volatile("vcvtph2ps %1, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(input) : "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == expected[i],
                    "VCVTPH2PS memory lane %d", i);

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\t"
                     "vcvtph2ps %2, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(input) : "ymm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u32[i] == expected[i],
                    "VCVTPH2PS xmm lane %d", i);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(out.u32[i] == 0, "VCVTPH2PS VEX.128 upper lane %d", i);
    TEST_END();
}
#endif

#endif
