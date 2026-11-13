/* AVX-512 floating-point comparisons, including unordered NaN predicates. */
#include "avx512_test.h"
#include <math.h>

#define RUN_CMPPS(imm, result) \
    __asm__ volatile("vcmpps $" #imm ", %%zmm1, %%zmm0, %%k1\n\t" \
                     "kmovw %%k1, %0" \
                     : "=r"(result) : : "zmm0", "zmm1", "k1")

static void test_packed_predicates(void) {
    zmm_test_t a, b;
    uint32_t mask;
    for (int i = 0; i < 16; i++) {
        a.f32[i] = (float)i;
        b.f32[i] = (float)i;
    }
    a.f32[0] = NAN;
    b.f32[1] = NAN;
    a.f32[2] = -INFINITY;
    b.f32[2] = INFINITY;
    a.f32[3] = 2.0f;
    b.f32[3] = 1.0f;
    a.f32[15] = -0.0f;
    b.f32[15] = 0.0f;

    __asm__ volatile("vmovaps %0, %%zmm0\n\tvmovaps %1, %%zmm1"
                     : : "m"(a), "m"(b) : "zmm0", "zmm1");
    RUN_CMPPS(0, mask); /* EQ_OQ */
    TEST_ASSERT(mask == 0xfff0U, "VCMPPS EQ_OQ mask 0x%04x", mask);
    RUN_CMPPS(1, mask); /* LT_OS */
    TEST_ASSERT(mask == 0x0004U, "VCMPPS LT_OS mask 0x%04x", mask);
    RUN_CMPPS(3, mask); /* UNORD_Q */
    TEST_ASSERT(mask == 0x0003U, "VCMPPS UNORD_Q mask 0x%04x", mask);
    RUN_CMPPS(4, mask); /* NEQ_UQ */
    TEST_ASSERT(mask == 0x000fU, "VCMPPS NEQ_UQ mask 0x%04x", mask);
    RUN_CMPPS(7, mask); /* ORD_Q */
    TEST_ASSERT(mask == 0xfffcU, "VCMPPS ORD_Q mask 0x%04x", mask);

    /* A writemask on a k destination clears masked-off result bits. */
    uint32_t input_mask = 0x8009U;
    uint32_t output_mask;
    __asm__ volatile(
        "kmovw %3, %%k2\n\t"
        "vmovaps %1, %%zmm0\n\tvmovaps %2, %%zmm1\n\t"
        "vcmpps $0, %%zmm1, %%zmm0, %%k1%{%%k2%}\n\t"
        "kmovw %%k1, %0"
        : "=r"(output_mask) : "m"(a), "m"(b), "r"(input_mask)
        : "zmm0", "zmm1", "k1", "k2");
    TEST_ASSERT(output_mask == 0x8000U,
                "VCMPPS writemask keeps only selected true lanes: 0x%04x", output_mask);
}

static void test_double_and_scalar(void) {
    zmm_test_t a = {0}, b = {0};
    uint32_t mask;
    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)i;
        b.f64[i] = (double)i;
    }
    a.f64[0] = NAN;
    b.f64[7] = NAN;
    __asm__ volatile(
        "vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
        "vcmppd $3, %%zmm1, %%zmm0, %%k1\n\t"
        "kmovb %%k1, %0"
        : "=r"(mask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT((mask & 0xffU) == 0x81U, "VCMPPD UNORD_Q first/last lanes: 0x%02x", mask);

    xmm_t x = {.f32 = {NAN, 11.0f, 22.0f, 33.0f}};
    xmm_t y = {.f32 = {1.0f, -1.0f, -2.0f, -3.0f}};
    __asm__ volatile(
        "vmovaps %1, %%xmm0\n\tvmovaps %2, %%xmm1\n\t"
        "vcmpss $3, %%xmm1, %%xmm0, %%k1\n\t"
        "kmovb %%k1, %0"
        : "=r"(mask) : "m"(x), "m"(y) : "xmm0", "xmm1", "k1");
    TEST_ASSERT((mask & 0xffU) == 1U, "VCMPSS unordered scalar result 0x%02x", mask);

    x.f64[0] = -0.0;
    y.f64[0] = 0.0;
    __asm__ volatile(
        "vmovapd %1, %%xmm0\n\tvmovapd %2, %%xmm1\n\t"
        "vcmpsd $0, %%xmm1, %%xmm0, %%k1\n\t"
        "kmovb %%k1, %0"
        : "=r"(mask) : "m"(x), "m"(y) : "xmm0", "xmm1", "k1");
    TEST_ASSERT((mask & 0xffU) == 1U, "VCMPSD treats signed zeros as equal");
}

int main(void) {
    TEST_START("AVX-512 floating-point compare predicates");
    if (!CHECK_AVX512F()) return 0;
    test_packed_predicates();
    test_double_and_scalar();
    TEST_END();
}
