/* AVX-512FP16 packed half arithmetic, compare, FMA, and square root. */
#include "../include/avx512_test.h"
#include <math.h>

static uint16_t float_to_half(float value) {
    uint32_t bits;
    memcpy(&bits, &value, 4);
    uint32_t sign = (bits >> 16) & 0x8000U;
    uint32_t exponent = (bits >> 23) & 0xffU;
    uint32_t mantissa = bits & 0x7fffffU;
    if (exponent == 0xffU)
        return (uint16_t)(sign | 0x7c00U | (mantissa ? 0x0200U | (mantissa >> 13) : 0));
    int half_exp = (int)exponent - 127 + 15;
    if (half_exp >= 31) return (uint16_t)(sign | 0x7c00U);
    if (half_exp <= 0) {
        if (half_exp < -10) return (uint16_t)sign;
        mantissa |= 0x800000U;
        unsigned shift = (unsigned)(14 - half_exp);
        uint32_t rounded = mantissa + ((1U << (shift - 1)) - 1U) + ((mantissa >> shift) & 1U);
        return (uint16_t)(sign | (rounded >> shift));
    }
    uint32_t rounded = mantissa + 0x0fffU + ((mantissa >> 13) & 1U);
    if (rounded & 0x800000U) { rounded = 0; half_exp++; }
    if (half_exp >= 31) return (uint16_t)(sign | 0x7c00U);
    return (uint16_t)(sign | ((uint32_t)half_exp << 10) | (rounded >> 13));
}

static float half_to_float(uint16_t half) {
    uint32_t sign = (uint32_t)(half & 0x8000U) << 16;
    uint32_t exponent = (half >> 10) & 0x1fU;
    uint32_t mantissa = half & 0x03ffU;
    uint32_t bits;
    if (exponent == 0) {
        if (mantissa == 0) bits = sign;
        else {
            int exp = -14;
            while ((mantissa & 0x0400U) == 0) { mantissa <<= 1; exp--; }
            mantissa &= 0x03ffU;
            bits = sign | (uint32_t)(exp + 127) << 23 | mantissa << 13;
        }
    } else if (exponent == 31) {
        bits = sign | 0x7f800000U | mantissa << 13;
    } else {
        bits = sign | (exponent - 15U + 127U) << 23 | mantissa << 13;
    }
    float value;
    memcpy(&value, &bits, 4);
    return value;
}

int main(void) {
    TEST_START("AVX-512FP16 packed arithmetic");
    if (!CHECK_AVX512FP16()) return 0;
    zmm_test_t a, b, add, mul, fma, root;
    for (int i = 0; i < 32; i++) {
        a.u16[i] = float_to_half((float)(i + 1));
        b.u16[i] = float_to_half(2.0f);
    }
    a.u16[0] = float_to_half(-0.0f);
    a.u16[1] = float_to_half(INFINITY);
    a.u16[2] = UINT16_C(0x7e01);
    __asm__ volatile("vmovdqa64 %4, %%zmm0\n\tvmovdqa64 %5, %%zmm1\n\t"
                     "vaddph %%zmm1, %%zmm0, %%zmm2\n\tvmulph %%zmm1, %%zmm0, %%zmm3\n\t"
                     "vmovdqa64 %4, %%zmm4\n\tvfmadd231ph %%zmm1, %%zmm1, %%zmm4\n\t"
                     "vsqrtph %%zmm0, %%zmm5\n\tvmovdqa64 %%zmm2, %0\n\t"
                     "vmovdqa64 %%zmm3, %1\n\tvmovdqa64 %%zmm4, %2\n\tvmovdqa64 %%zmm5, %3"
                     : "=m"(add), "=m"(mul), "=m"(fma), "=m"(root)
                     : "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5");
    for (int i = 3; i < 32; i++) {
        float av = half_to_float(a.u16[i]);
        TEST_ASSERT(add.u16[i] == float_to_half(av + 2.0f), "VADDPH lane %d", i);
        TEST_ASSERT(mul.u16[i] == float_to_half(av * 2.0f), "VMULPH lane %d", i);
        TEST_ASSERT(fma.u16[i] == float_to_half(av + 4.0f), "VFMADD231PH lane %d", i);
        TEST_ASSERT(root.u16[i] == float_to_half(sqrtf(av)), "VSQRTPH lane %d", i);
    }
    uint32_t mask;
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvcmpph $3, %%zmm0, %%zmm0, %%k1\n\t"
                     "kmovd %%k1, %0" : "=r"(mask) : "m"(a) : "zmm0", "k1");
    TEST_ASSERT((mask & 0xffffffffU) == (UINT32_C(1) << 2), "VCMPPH unordered QNaN lane");
    TEST_ASSERT(add.u16[1] == float_to_half(INFINITY), "VADDPH infinity boundary");
    TEST_END();
}
