/* TARGET: VPSHLDD */
#include "../include/avx512_test.h"

static uint32_t shld32(uint32_t a, uint32_t b, unsigned count) {
    count &= 31U;
    return count ? (a << count) | (b >> (32U - count)) : a;
}

#define RUN_IMMEDIATE(IMM) do {                                                \
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa32 %2, %%zmm1\n\t"       \
                     "vpshldd $" #IMM ", %%zmm1, %%zmm0, %%zmm2\n\t"       \
                     "vmovdqa32 %%zmm2, %0"                                  \
                     : "=m"(out) : "m"(a), "m"(b)                           \
                     : "zmm0", "zmm1", "zmm2");                            \
    for (int lane = 0; lane < 16; lane++)                                     \
        TEST_ASSERT(out.u32[lane] == shld32(a.u32[lane], b.u32[lane], IMM),   \
                    "VPSHLDD immediate " #IMM " lane %d", lane);             \
} while (0)

int main(void) {
    zmm_test_t a, b, initial, out;
    uint32_t mask;

    TEST_START("VPSHLDD");
    if (!CHECK_AVX512VBMI2()) return 0;
    for (int i = 0; i < 16; i++) {
        a.u32[i] = UINT32_C(0x80000001) ^ (UINT32_C(0x10204081) * (uint32_t)i);
        b.u32[i] = UINT32_C(0x7ffffffe) ^ (UINT32_C(0x01020408) * (uint32_t)i);
        initial.u32[i] = UINT32_C(0xdeadbeef);
    }
    RUN_IMMEDIATE(0);
    RUN_IMMEDIATE(1);
    RUN_IMMEDIATE(31);
    RUN_IMMEDIATE(32);
    RUN_IMMEDIATE(255);

    mask = UINT32_C(0x8001);
    __asm__ volatile("kmovw %4, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\tvmovdqa32 %3, %%zmm2\n\t"
                     "vpshldd $4, %%zmm1, %%zmm0, %%zmm2%{%%k1%}\n\t"
                     "vmovdqa32 %%zmm2, %0"
                     : "=m"(out)
                     : "m"(a), "m"(b), "m"(initial), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U
            ? shld32(a.u32[i], b.u32[i], 4) : UINT32_C(0xdeadbeef);
        TEST_ASSERT(out.u32[i] == expected, "VPSHLDD merge mask lane %d", i);
    }

    mask = UINT32_C(0x5aa5);
    __asm__ volatile("kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqa32 %2, %%zmm1\n\t"
                     "vpshldd $4, %%zmm1, %%zmm0, %%zmm2%{%%k1%}%{z%}\n\t"
                     "vmovdqa32 %%zmm2, %0"
                     : "=m"(out) : "m"(a), "m"(b), "r"(mask)
                     : "zmm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 16; i++) {
        uint32_t expected = (mask >> i) & 1U ? shld32(a.u32[i], b.u32[i], 4) : 0;
        TEST_ASSERT(out.u32[i] == expected, "VPSHLDD zero mask lane %d", i);
    }
    TEST_END();
}

#undef RUN_IMMEDIATE
