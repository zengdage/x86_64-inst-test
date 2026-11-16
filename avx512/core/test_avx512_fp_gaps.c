/* AVX-512F/DQ instructions missing from the wider suite:
 * VSQRTPD, VPERMPS, VSHUFF64X2, the EVEX legacy-predicate compares
 * (VCMPEQPS/PD/SS/SD), the EVEX scalar compares (VCMPSS/VCMPSD), VRANGESD,
 * VSCALEFSD and the EVEX VBROADCASTSD.
 */
#include "../include/avx512_test.h"
#include <float.h>
#include <math.h>

static void run_sqrtpd(const double *values, const double *expected) {
    zmm_test_t src, initial, plain, merged, zeroed;
    memcpy(src.f64, values, 8 * sizeof(double));
    for (int i = 0; i < 8; i++) initial.u64[i] = UINT64_C(0xaaaaaaaabbbbbbbb);

    uint32_t mask = UINT32_C(0x000f);
    __asm__ volatile(
        "kmovd %3, %%k1\n\t"
        "vmovapd %4, %%zmm0\n\tvsqrtpd %%zmm0, %%zmm1\n\t"
        "vmovapd %5, %%zmm2\n\tvsqrtpd %%zmm0, %%zmm2%{%%k1%}\n\t"
        "vmovapd %5, %%zmm3\n\tvsqrtpd %%zmm0, %%zmm3%{%%k1%}%{z%}\n\t"
        "vmovapd %%zmm1, %0\n\tvmovapd %%zmm2, %1\n\tvmovapd %%zmm3, %2"
        : "=m"(plain), "=m"(merged), "=m"(zeroed)
        : "r"(mask), "m"(src), "m"(initial)
        : "zmm0", "zmm1", "zmm2", "zmm3", "k1");

    for (int i = 0; i < 8; i++) {
        if (isnan(expected[i])) {
            TEST_ASSERT(IS_QNAN(plain.f64[i]), "VSQRTPD lane %d is QNaN", i);
        } else {
            TEST_ASSERT(plain.f64[i] == expected[i], "VSQRTPD lane %d = %g", i, plain.f64[i]);
        }
    }
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(merged.u64[i] == plain.u64[i], "VSQRTPD merge mask lane %d", i);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(merged.u64[i] == initial.u64[i], "VSQRTPD merge keeps old lane %d", i);
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(zeroed.u64[i] == plain.u64[i], "VSQRTPD zero mask lane %d", i);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(zeroed.u64[i] == 0, "VSQRTPD zero mask clears lane %d", i);
}

static void test_sqrtpd(void) {
    const double first[8] = {0.0, -0.0, 4.0, 9.0, 16.0, 25.0, INFINITY, -INFINITY};
    const double first_expected[8] = {0.0, -0.0, 2.0, 3.0, 4.0, 5.0, INFINITY, NAN};
    const double second[8] = {NAN, -1.0, 2.0, 100.0, 0.25, 1e10, 0x1p-1074, 1.0};
    const double second_expected[8] = {NAN, NAN, sqrt(2.0), 10.0, 0.5, sqrt(1e10),
                                       sqrt(0x1p-1074), 1.0};
    run_sqrtpd(first, first_expected);
    run_sqrtpd(second, second_expected);

    /* Signed zero must survive the square root unchanged. */
    zmm_test_t src, out;
    src.f64[0] = 0.0;
    src.f64[1] = -0.0;
    __asm__ volatile("vmovapd %1, %%xmm0\n\tvsqrtpd %%zmm0, %%zmm1\n\t"
                     "vmovapd %%zmm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "zmm1");
    TEST_ASSERT(out.f64[0] == 0.0 && !signbit(out.f64[0]), "VSQRTPD +0 keeps positive sign");
    TEST_ASSERT(out.f64[1] == 0.0 && signbit(out.f64[1]), "VSQRTPD -0 keeps negative sign");

    /* Narrow widths must produce the same values as the 512-bit form. */
    xmm_t lo;
    __asm__ volatile("vmovapd %1, %%xmm0\n\tvsqrtpd %%xmm0, %%xmm1\n\tvmovapd %%xmm1, %0"
                     : "=m"(lo) : "m"(src) : "xmm0", "xmm1");
    TEST_ASSERT(lo.f64[0] == 0.0, "VSQRTPD xmm lane 0");
    TEST_ASSERT(lo.f64[1] == 0.0 && signbit(lo.f64[1]), "VSQRTPD xmm lane 1");
}

static void test_permps(void) {
    zmm_test_t src, idx, out, merged, zeroed, initial;
    for (int i = 0; i < 16; i++) src.f32[i] = (float)(i + 1) * 0.5f;
    for (int i = 0; i < 16; i++) idx.u32[i] = (uint32_t)(15 - i);
    /* Out-of-range and high-bit indices select modulo the element count. */
    idx.u32[0] = 16;
    idx.u32[1] = UINT32_MAX;
    idx.u32[2] = 0x80000000U | 3U;

    /* VPERMPS takes the data vector first and the index vector second. */
    __asm__ volatile("vmovaps %1, %%zmm0\n\tvmovaps %2, %%zmm1\n\t"
                     "vpermps %%zmm0, %%zmm1, %%zmm2\n\tvmovaps %%zmm2, %0"
                     : "=m"(out) : "m"(src), "m"(idx) : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++) {
        float expected = src.f32[idx.u32[i] & 15];
        TEST_ASSERT(out.f32[i] == expected, "VPERMPS lane %d", i);
    }

    uint32_t mask = UINT32_C(0x8001);
    for (int i = 0; i < 16; i++) initial.f32[i] = -777.0f;
    __asm__ volatile("kmovd %2, %%k1\n\tvmovaps %3, %%zmm0\n\tvmovaps %4, %%zmm1\n\t"
                     "vmovaps %5, %%zmm2\n\tvmovaps %5, %%zmm3\n\t"
                     "vpermps %%zmm0, %%zmm1, %%zmm2%{%%k1%}\n\tvmovaps %%zmm2, %0\n\t"
                     "vpermps %%zmm0, %%zmm1, %%zmm3%{%%k1%}%{z%}\n\tvmovaps %%zmm3, %1"
                     : "=m"(merged), "=m"(zeroed)
                     : "r"(mask), "m"(src), "m"(idx), "m"(initial)
                     : "zmm0", "zmm1", "zmm2", "zmm3", "k1");
    for (int i = 0; i < 16; i++) {
        float expected = src.f32[idx.u32[i] & 15];
        TEST_ASSERT(merged.f32[i] == (((mask >> i) & 1) ? expected : initial.f32[i]),
                    "VPERMPS merge mask lane %d", i);
        TEST_ASSERT(zeroed.f32[i] == (((mask >> i) & 1) ? expected : 0.0f),
                    "VPERMPS zero mask lane %d", i);
    }
}

/* VSHUFF64X2 moves 128-bit lanes: imm[1:0] and imm[3:2] pick the two
 * destination lanes taken from the first source, imm[5:4] and imm[7:6] pick
 * the two destination lanes taken from the second source.
 */
#define CHECK_SHUFF(IMM) do { \
    __asm__ volatile("vmovdqu64 %1, %%zmm0\n\tvmovdqu64 %2, %%zmm1\n\t" \
                     "vshuff64x2 $" #IMM ", %%zmm0, %%zmm1, %%zmm2\n\t" \
                     "vmovdqu64 %%zmm2, %0" \
                     : "=m"(out) : "m"(src1), "m"(src2) : "zmm0", "zmm1", "zmm2"); \
    for (int l = 0; l < 4; l++) { \
        int sel = ((IMM) >> (l * 2)) & 3; \
        const zmm_test_t *base = (l < 2) ? &src2 : &src1; \
        for (int q = 0; q < 2; q++) \
            TEST_ASSERT(out.u64[l * 2 + q] == base->u64[sel * 2 + q], \
                        "VSHUFF64X2 imm " #IMM " lane %d qword %d", l, q); \
    } \
} while (0)

static void test_shuff64x2(void) {
    zmm_test_t src1, src2, out;
    for (int i = 0; i < 8; i++) {
        src1.u64[i] = UINT64_C(0x1000000000000000) + (uint64_t)i;
        src2.u64[i] = UINT64_C(0x2000000000000000) + (uint64_t)i;
    }
    CHECK_SHUFF(0x00);
    CHECK_SHUFF(0x01);
    CHECK_SHUFF(0x02);
    CHECK_SHUFF(0x03);
    CHECK_SHUFF(0x04);
    CHECK_SHUFF(0x09);
    CHECK_SHUFF(0x0e);
    CHECK_SHUFF(0x0f);
    CHECK_SHUFF(0x10);
    CHECK_SHUFF(0x21);
    CHECK_SHUFF(0x36);
    CHECK_SHUFF(0x4b);
    CHECK_SHUFF(0x5c);
    CHECK_SHUFF(0x6d);
    CHECK_SHUFF(0x7e);
    CHECK_SHUFF(0x8f);
}

static void test_cmpeq_legacy(void) {
    zmm_test_t a, b;
    uint32_t ps_mask = 0;
    uint64_t pd_mask = 0;
    for (int i = 0; i < 16; i++) {
        a.f32[i] = (float)i;
        b.f32[i] = (float)((i == 5) ? 5 : i + 1);
    }
    __asm__ volatile("vmovaps %1, %%zmm0\n\tvmovaps %2, %%zmm1\n\t"
                     "vcmpeqps %%zmm0, %%zmm1, %%k1\n\tkmovd %%k1, %0"
                     : "=r"(ps_mask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(ps_mask == (1U << 5), "VCMPEQPS EVEX equality mask 0x%08x", ps_mask);

    for (int i = 0; i < 8; i++) {
        a.f64[i] = (double)i;
        b.f64[i] = (double)((i == 3) ? 3 : i + 1);
    }
    __asm__ volatile("vmovapd %1, %%zmm0\n\tvmovapd %2, %%zmm1\n\t"
                     "vcmpeqpd %%zmm0, %%zmm1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(pd_mask) : "m"(a), "m"(b) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(pd_mask == (1U << 3), "VCMPEQPD EVEX equality mask 0x%08llx",
                (unsigned long long)pd_mask);

    float fa = 1.0f, fb = 1.0f, fc = 2.0f;
    uint64_t ss_mask = 0, sd_mask = 0;
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\t"
                     "vcmpeqss %%xmm0, %%xmm1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(ss_mask) : "m"(fa), "m"(fb) : "xmm0", "xmm1", "k1");
    TEST_ASSERT(ss_mask == 1U, "VCMPEQSS EVEX equality mask %llu", (unsigned long long)ss_mask);
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\t"
                     "vcmpeqss %%xmm0, %%xmm1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(ss_mask) : "m"(fa), "m"(fc) : "xmm0", "xmm1", "k1");
    TEST_ASSERT(ss_mask == 0U, "VCMPEQSS EVEX inequality mask %llu", (unsigned long long)ss_mask);

    double da = 2.5, db = 2.5;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\t"
                     "vcmpeqsd %%xmm0, %%xmm1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(sd_mask) : "m"(da), "m"(db) : "xmm0", "xmm1", "k1");
    TEST_ASSERT(sd_mask == 1U, "VCMPEQSD EVEX equality mask %llu", (unsigned long long)sd_mask);
}

#define RUN_SCALAR_CMP(IMM, EXPECTED, LABEL) do { \
    uint64_t mask = 0; \
    __asm__ volatile("vmovsd %2, %%xmm0\n\tvmovsd %1, %%xmm1\n\t" \
                     "vcmpsd $" #IMM ", %%xmm0, %%xmm1, %%k1\n\tkmovq %%k1, %0" \
                     : "=r"(mask) : "m"(left), "m"(right) : "xmm0", "xmm1", "k1"); \
    TEST_ASSERT(mask == (EXPECTED), LABEL " imm " #IMM " mask %llu", \
                (unsigned long long)mask); \
} while (0)

static void test_cmpsd_cmpss(void) {
    double left = 1.0, right = 2.0;
    RUN_SCALAR_CMP(0, 0U, "VCMPSD EQ");
    RUN_SCALAR_CMP(1, 1U, "VCMPSD LT");
    RUN_SCALAR_CMP(2, 1U, "VCMPSD LE");
    RUN_SCALAR_CMP(3, 0U, "VCMPSD UNORD");
    RUN_SCALAR_CMP(4, 1U, "VCMPSD NEQ");
    RUN_SCALAR_CMP(5, 0U, "VCMPSD NLT");
    RUN_SCALAR_CMP(6, 0U, "VCMPSD NLE");
    RUN_SCALAR_CMP(7, 1U, "VCMPSD ORD");

    left = 2.0;
    right = 1.0;
    RUN_SCALAR_CMP(1, 0U, "VCMPSD LT reversed");
    RUN_SCALAR_CMP(2, 0U, "VCMPSD LE reversed");
    RUN_SCALAR_CMP(5, 1U, "VCMPSD NLT reversed");
    RUN_SCALAR_CMP(6, 1U, "VCMPSD NLE reversed");

    /* NaN participates only in the unordered/ordered predicates. */
    right = NAN;
    RUN_SCALAR_CMP(0, 0U, "VCMPSD EQ NaN");
    RUN_SCALAR_CMP(3, 1U, "VCMPSD UNORD NaN");
    RUN_SCALAR_CMP(4, 1U, "VCMPSD NEQ NaN");
    RUN_SCALAR_CMP(7, 0U, "VCMPSD ORD NaN");

    float fleft = 1.0f, fright = 2.0f;
    uint64_t mask = 0;
    __asm__ volatile("vmovss %2, %%xmm0\n\tvmovss %1, %%xmm1\n\t"
                     "vcmpss $1, %%xmm0, %%xmm1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(mask) : "m"(fleft), "m"(fright) : "xmm0", "xmm1", "k1");
    TEST_ASSERT(mask == 1U, "VCMPSS LT mask %llu", (unsigned long long)mask);
    __asm__ volatile("vmovss %2, %%xmm0\n\tvmovss %1, %%xmm1\n\t"
                     "vcmpss $4, %%xmm0, %%xmm1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(mask) : "m"(fleft), "m"(fright) : "xmm0", "xmm1", "k1");
    TEST_ASSERT(mask == 1U, "VCMPSS NEQ mask %llu", (unsigned long long)mask);
    fright = NAN;
    __asm__ volatile("vmovss %2, %%xmm0\n\tvmovss %1, %%xmm1\n\t"
                     "vcmpss $3, %%xmm0, %%xmm1, %%k1\n\tkmovq %%k1, %0"
                     : "=r"(mask) : "m"(fleft), "m"(fright) : "xmm0", "xmm1", "k1");
    TEST_ASSERT(mask == 1U, "VCMPSS UNORD NaN mask %llu", (unsigned long long)mask);
}

/* VRANGE semantics: imm8[1:0] selects signed minimum (0), signed maximum (1),
 * minimum magnitude (2) or maximum magnitude (3).  imm8[2] selects whether
 * the result carries the sign of the first source operand (0) or the sign of
 * the selected value (1); imm8[3] forces the result to be non-negative (0) or
 * non-positive (1) when imm8[2] selects a fixed polarity.
 */
#define RUN_RANGE(IMM, EXPECTED, LABEL) do { \
    double out = 0.0; \
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\t" \
                     "vrangesd $" #IMM ", %%xmm1, %%xmm0, %%xmm2\n\t" \
                     "vmovsd %%xmm2, %0" \
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2"); \
    TEST_ASSERT(out == (EXPECTED), LABEL " imm " #IMM " = %g", out); \
} while (0)

static void test_rangesd(void) {
    double a = 3.0, b = -5.0;
    RUN_RANGE(0, 5.0, "VRANGESD signed minimum keeps the value sign");
    RUN_RANGE(1, 3.0, "VRANGESD signed maximum keeps the value sign");
    RUN_RANGE(2, 3.0, "VRANGESD minimum magnitude keeps the value sign");
    RUN_RANGE(3, 5.0, "VRANGESD maximum magnitude keeps the value sign");

    a = -2.0; b = 4.0;
    RUN_RANGE(0, -2.0, "VRANGESD signed minimum negative value");
    RUN_RANGE(1, -4.0, "VRANGESD signed maximum negative value");
    RUN_RANGE(4, -2.0, "VRANGESD keeps the sign of the selected value");
    RUN_RANGE(8, 2.0, "VRANGESD forced non-negative result");
    RUN_RANGE(12, -2.0, "VRANGESD forced non-positive result");

    a = 5.0; b = 5.0;
    RUN_RANGE(0, 5.0, "VRANGESD equal values minimum");
    RUN_RANGE(12, -5.0, "VRANGESD equal values non-positive");
}

#undef RUN_RANGE

static void test_scalefsd(void) {
    double base = 3.0, scale = 2.0, out;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\t"
                     "vscalefsd %%xmm1, %%xmm0, %%xmm2\n\tvmovsd %%xmm2, %0"
                     : "=m"(out) : "m"(base), "m"(scale) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(out == 12.0, "VSCALEFSD 3.0*2^2 = %g", out);

    scale = -1.0;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\t"
                     "vscalefsd %%xmm1, %%xmm0, %%xmm2\n\tvmovsd %%xmm2, %0"
                     : "=m"(out) : "m"(base), "m"(scale) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(out == 1.5, "VSCALEFSD 3.0*2^-1 = %g", out);

    scale = 0.0;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\t"
                     "vscalefsd %%xmm1, %%xmm0, %%xmm2\n\tvmovsd %%xmm2, %0"
                     : "=m"(out) : "m"(base), "m"(scale) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(out == 3.0, "VSCALEFSD 3.0*2^0 = %g", out);
}

static void test_broadcastsd(void) {
    double value = 2.75;
    zmm_test_t out;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvbroadcastsd %%xmm0, %%zmm1\n\t"
                     "vmovapd %%zmm1, %0"
                     : "=m"(out) : "m"(value) : "xmm0", "zmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.f64[i] == 2.75, "VBROADCASTSD lane %d", i);
}

int main(void) {
    TEST_START("AVX-512 FP gap instructions");
    if (!CHECK_AVX512F()) return 0;
    test_sqrtpd();
    test_permps();
    test_shuff64x2();
    test_cmpeq_legacy();
    test_cmpsd_cmpss();
    test_rangesd();
    test_scalefsd();
    test_broadcastsd();
    TEST_END();
}
