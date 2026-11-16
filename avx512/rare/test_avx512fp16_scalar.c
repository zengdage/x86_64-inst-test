/* AVX-512FP16 scalar and mixed packed forms omitted by the packed smoke test. */
#include "../include/avx512_test.h"
#include <math.h>

static uint16_t f32_to_h(float value) {
    uint32_t bits; memcpy(&bits, &value, 4);
    uint32_t sign = (bits >> 16) & 0x8000U, exp = (bits >> 23) & 0xffU, mant = bits & 0x7fffffU;
    if (exp == 0xffU) return (uint16_t)(sign | 0x7c00U | (mant ? 0x0200U : 0));
    int e = (int)exp - 112;
    if (e >= 31) return (uint16_t)(sign | 0x7c00U);
    if (e <= 0) return (uint16_t)sign;
    mant += 0x0fffU + ((mant >> 13) & 1U);
    if (mant & 0x800000U) { mant = 0; e++; }
    return (uint16_t)(sign | ((uint32_t)e << 10) | (mant >> 13));
}

static void test_scalar_arithmetic(void) {
    zmm_test_t a = {0}, b = {0}, out;
    a.u16[0] = f32_to_h(1.5f); b.u16[0] = f32_to_h(2.0f);
#define TEST_SCALAR(INST, EXPECTED, LABEL) do { \
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t" \
                     #INST " %%xmm1, %%xmm0, %%xmm0\n\tvmovdqa64 %%zmm0, %0" \
                     : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1"); \
    TEST_ASSERT(out.u16[0] == f32_to_h(EXPECTED), LABEL); \
} while (0)
    TEST_SCALAR(vaddsh, 3.5f, "VADDSH");
    TEST_SCALAR(vsubsh, -0.5f, "VSUBSH");
    TEST_SCALAR(vmulsh, 3.0f, "VMULSH");
    TEST_SCALAR(vdivsh, 0.75f, "VDIVSH");
    TEST_SCALAR(vsqrtsh, sqrtf(2.0f), "VSQRTSH uses the second source");
#undef TEST_SCALAR

    a.u16[0] = f32_to_h(NAN); b.u16[0] = f32_to_h(1.0f);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\tvaddsh %%xmm1, %%xmm0, %%xmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1");
    TEST_ASSERT((out.u16[0] & 0x7c00U) == 0x7c00U && (out.u16[0] & 0x03ffU), "VADDSH QNaN");
    a.u16[0] = f32_to_h(-0.0f); b.u16[0] = f32_to_h(0.0f);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\tvaddsh %%xmm1, %%xmm0, %%xmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1");
    TEST_ASSERT(out.u16[0] == 0, "VADDSH signed zero cancellation");
}

static void test_scalar_predicates_and_exponents(void) {
    zmm_test_t a = {0}, b = {0}, out;
    a.u16[0] = f32_to_h(NAN); b.u16[0] = f32_to_h(1.0f);
    uint32_t mask;
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\tvcmpsh $3, %%xmm1, %%xmm0, %%k1\n\tkmovw %%k1, %0"
                     : "=r"(mask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(mask & 1U, "VCMPSH unordered NaN");
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvgetexpsh %%xmm0, %%xmm0, %%xmm1\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(a) : "zmm0", "zmm1");
    TEST_ASSERT((out.u16[0] & 0x7c00U) == 0x7c00U, "VGETEXPSH NaN exponent");
    a.u16[0] = f32_to_h(6.0f);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvgetmantsh $0, %%xmm0, %%xmm0, %%xmm1\n\tvmovdqa64 %%zmm1, %0"
                     : "=m"(out) : "m"(a) : "zmm0", "zmm1");
    TEST_ASSERT(out.u16[0] >= f32_to_h(1.0f) && out.u16[0] < f32_to_h(2.0f), "VGETMANTSH normalized");
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvfpclasssh $0x80, %%xmm0, %%k1\n\tkmovw %%k1, %0"
                     : "=r"(mask) : "m"(a) : "zmm0", "k1");
    TEST_ASSERT((mask & 1U) == 0, "VFPCLASSSH finite value");
}

static void test_scalar_fma(void) {
    zmm_test_t a = {0}, b = {0}, c = {0}, out;
    a.u16[0] = f32_to_h(2.0f); b.u16[0] = f32_to_h(3.0f); c.u16[0] = f32_to_h(4.0f);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
                     "vfmadd132sh %%xmm2, %%xmm1, %%xmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(out.u16[0] == f32_to_h(11.0f), "VFMADD132SH");
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\tvmovdqa64 %3, %%zmm2\n\t"
                     "vfmaddcsh %%xmm2, %%xmm1, %%xmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(c) : "zmm0", "zmm1", "zmm2");
    TEST_ASSERT(out.u16[0] == f32_to_h(14.0f), "VFMADDC SH real lane");
}

int main(void) {
    TEST_START("AVX-512FP16 scalar arithmetic and predicates");
    if (!CHECK_AVX512FP16()) return 0;
    test_scalar_arithmetic();
    test_scalar_predicates_and_exponents();
    test_scalar_fma();
    TEST_END();
}
