/* AVX-512 VFIXUPIMM packed-double and scalar forms.  The control dword has
 * one response nibble for each source classification: QNaN, SNaN, zero,
 * +1, -Inf, +Inf, negative finite and positive finite.
 */
#include "../include/avx512_test.h"

static uint64_t response_f64(unsigned action, uint64_t old_dst,
                             uint64_t source) {
    static const uint64_t constants[16] = {
        0, 0, 0, UINT64_C(0xfff8000000000000),
        UINT64_C(0xfff0000000000000), UINT64_C(0x7ff0000000000000),
        0, UINT64_C(0x8000000000000000),
        UINT64_C(0x0000000000000000), UINT64_C(0xbff0000000000000),
        UINT64_C(0x3ff0000000000000), UINT64_C(0x3fe0000000000000),
        UINT64_C(0x4056800000000000), UINT64_C(0x3ff921fb54442d18),
        UINT64_C(0x7fefffffffffffff), UINT64_C(0xffefffffffffffff)
    };
    if (action == 0) return old_dst;
    if (action == 1) return source;
    if (action == 2)
        return (source & UINT64_C(0x8007ffffffffffff)) |
               UINT64_C(0x7ff8000000000000);
    if (action == 6)
        return (source & UINT64_C(0x8000000000000000)) |
               UINT64_C(0x7ff0000000000000);
    return constants[action & 15U];
}

static uint32_t response_f32(unsigned action, uint32_t old_dst,
                             uint32_t source) {
    static const uint32_t constants[16] = {
        0, 0, 0, UINT32_C(0xffc00000),
        UINT32_C(0xff800000), UINT32_C(0x7f800000),
        0, UINT32_C(0x80000000),
        UINT32_C(0x00000000), UINT32_C(0xbf800000),
        UINT32_C(0x3f800000), UINT32_C(0x3f000000),
        UINT32_C(0x42b40000), UINT32_C(0x3fc90fdb),
        UINT32_C(0x7f7fffff), UINT32_C(0xff7fffff)
    };
    if (action == 0) return old_dst;
    if (action == 1) return source;
    if (action == 2)
        return (source & UINT32_C(0x803fffff)) | UINT32_C(0x7fc00000);
    if (action == 6)
        return (source & UINT32_C(0x80000000)) | UINT32_C(0x7f800000);
    return constants[action & 15U];
}

static void test_ps_response_table(void) {
    zmm_test_t old_dst, source, control, out;
    for (int i = 0; i < 16; i++) {
        old_dst.f32[i] = 40.0f + i;
        source.f32[i] = 2.0f + i;
    }

    for (unsigned action = 0; action < 16; action++) {
        uint32_t word = UINT32_C(0x11111111) * action;
        for (int i = 0; i < 16; i++) control.u32[i] = word;
        __asm__ volatile(
            "vmovaps %1, %%zmm0\n\t"
            "vmovaps %2, %%zmm1\n\t"
            "vmovdqa32 %3, %%zmm2\n\t"
            "vfixupimmps $0, %%zmm2, %%zmm1, %%zmm0\n\t"
            "vmovaps %%zmm0, %0"
            : "=m"(out)
            : "m"(old_dst), "m"(source), "m"(control)
            : "zmm0", "zmm1", "zmm2");
        for (int i = 0; i < 16; i++) {
            uint32_t expected = response_f32(action, old_dst.u32[i], source.u32[i]);
            TEST_ASSERT(out.u32[i] == expected,
                        "VFIXUPIMMPS response %u lane %d: %08x != %08x",
                        action, i, out.u32[i], expected);
        }
    }
}

static void test_ps_classification(void) {
    static const uint32_t classes[8] = {
        UINT32_C(0x7fc01234), /* QNaN */
        UINT32_C(0x7f801234), /* SNaN */
        UINT32_C(0x00000000), /* zero */
        UINT32_C(0x3f800000), /* +1 */
        UINT32_C(0xff800000), /* -Inf */
        UINT32_C(0x7f800000), /* +Inf */
        UINT32_C(0xc0000000), /* negative finite */
        UINT32_C(0x40000000)  /* positive finite */
    };
    zmm_test_t old_dst, source, control, out;
    for (int i = 0; i < 16; i++) {
        int classification = i & 7;
        old_dst.f32[i] = 40.0f + i;
        source.u32[i] = classes[classification];
        control.u32[i] = UINT32_C(0xa) << (classification * 4);
    }
    __asm__ volatile(
        "vmovaps %1, %%zmm0\n\t"
        "vmovaps %2, %%zmm1\n\t"
        "vmovdqa32 %3, %%zmm2\n\t"
        "vfixupimmps $0, %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovaps %%zmm0, %0"
        : "=m"(out)
        : "m"(old_dst), "m"(source), "m"(control)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == UINT32_C(0x3f800000),
                    "VFIXUPIMMPS classification lane %d", i);
}

static void test_pd_response_table(void) {
    zmm_test_t old_dst, source, control, out;
    for (int i = 0; i < 8; i++) {
        old_dst.u64[i] = UINT64_C(0x4045000000000000) + ((uint64_t)i << 44);
        source.u64[i] = UINT64_C(0x4000000000000000) + (uint64_t)i;
    }

    for (unsigned action = 0; action < 16; action++) {
        uint32_t word = UINT32_C(0x11111111) * action;
        for (int i = 0; i < 8; i++) control.u64[i] = word;
        __asm__ volatile(
            "vmovapd %1, %%zmm0\n\t"
            "vmovapd %2, %%zmm1\n\t"
            "vmovdqa64 %3, %%zmm2\n\t"
            "vfixupimmpd $0, %%zmm2, %%zmm1, %%zmm0\n\t"
            "vmovapd %%zmm0, %0"
            : "=m"(out)
            : "m"(old_dst), "m"(source), "m"(control)
            : "zmm0", "zmm1", "zmm2");
        for (int i = 0; i < 8; i++) {
            uint64_t expected = response_f64(action, old_dst.u64[i], source.u64[i]);
            TEST_ASSERT(out.u64[i] == expected,
                        "VFIXUPIMMPD response %u lane %d: %016llx != %016llx",
                        action, i, (unsigned long long)out.u64[i],
                        (unsigned long long)expected);
        }
    }
}

static void test_pd_classification(void) {
    static const uint64_t classes[8] = {
        UINT64_C(0x7ff8000000001234), /* QNaN */
        UINT64_C(0x7ff0000000001234), /* SNaN */
        UINT64_C(0x0000000000000000), /* zero */
        UINT64_C(0x3ff0000000000000), /* +1 */
        UINT64_C(0xfff0000000000000), /* -Inf */
        UINT64_C(0x7ff0000000000000), /* +Inf */
        UINT64_C(0xc000000000000000), /* negative finite */
        UINT64_C(0x4000000000000000)  /* positive finite */
    };
    zmm_test_t old_dst, source, control, out;
    for (int i = 0; i < 8; i++) {
        old_dst.f64[i] = 40.0 + i;
        source.u64[i] = classes[i];
        control.u64[i] = (uint64_t)UINT32_C(0xa) << (i * 4);
    }
    __asm__ volatile(
        "vmovapd %1, %%zmm0\n\t"
        "vmovapd %2, %%zmm1\n\t"
        "vmovdqa64 %3, %%zmm2\n\t"
        "vfixupimmpd $0, %%zmm2, %%zmm1, %%zmm0\n\t"
        "vmovapd %%zmm0, %0"
        : "=m"(out)
        : "m"(old_dst), "m"(source), "m"(control)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == UINT64_C(0x3ff0000000000000),
                    "VFIXUPIMMPD classification %d", i);
}

static void run_pd_mask(uint32_t mask, zmm_test_t *merged,
                        zmm_test_t *zeroed) {
    zmm_test_t old_dst, source, control;
    for (int i = 0; i < 8; i++) {
        old_dst.f64[i] = 100.0 + i;
        source.f64[i] = 2.0 + i;
        control.u64[i] = UINT32_C(0xcccccccc);
    }
    __asm__ volatile(
        "kmovb %3, %%k1\n\t"
        "vmovapd %4, %%zmm0\n\t"
        "vmovapd %5, %%zmm1\n\t"
        "vmovdqa64 %6, %%zmm2\n\t"
        "vfixupimmpd $0, %%zmm2, %%zmm1, %%zmm0%{%%k1%}\n\t"
        "vmovapd %%zmm0, %0\n\t"
        "vmovapd %4, %%zmm0\n\t"
        "vfixupimmpd $0, %%zmm2, %%zmm1, %%zmm0%{%%k1%}%{z%}\n\t"
        "vmovapd %%zmm0, %1"
        : "=m"(*merged), "=m"(*zeroed)
        : "m"(old_dst), "r"(mask), "m"(old_dst), "m"(source), "m"(control)
        : "zmm0", "zmm1", "zmm2", "k1");
}

static void test_pd_masks(void) {
    static const uint32_t masks[] = {0x00, 0xff, 0x01, 0x80, 0xa5};
    zmm_test_t merged, zeroed;
    for (unsigned m = 0; m < sizeof(masks) / sizeof(masks[0]); m++) {
        run_pd_mask(masks[m], &merged, &zeroed);
        for (int i = 0; i < 8; i++) {
            int active = (masks[m] >> i) & 1U;
            double merge_expected = active ? 90.0 : 100.0 + i;
            double zero_expected = active ? 90.0 : 0.0;
            TEST_ASSERT(merged.f64[i] == merge_expected,
                        "VFIXUPIMMPD merge mask 0x%02x lane %d", masks[m], i);
            TEST_ASSERT(zeroed.f64[i] == zero_expected,
                        "VFIXUPIMMPD zero mask 0x%02x lane %d", masks[m], i);
        }
    }
}

static void test_scalar_single(void) {
    xmm_t old_dst, source, control, out;
    old_dst.u32[0] = UINT32_C(0x3f800000);
    old_dst.u32[1] = UINT32_C(0xaaaaaaaa);
    old_dst.u32[2] = UINT32_C(0xbbbbbbbb);
    old_dst.u32[3] = UINT32_C(0xcccccccc);
    source.u32[0] = UINT32_C(0x40000000);
    source.u32[1] = UINT32_C(0x11111111);
    source.u32[2] = UINT32_C(0x22222222);
    source.u32[3] = UINT32_C(0x33333333);
    control.u32[0] = UINT32_C(0xcccccccc);

    __asm__ volatile(
        "vmovdqa %1, %%xmm0\n\tvmovdqa %2, %%xmm1\n\t"
        "vmovdqa %3, %%xmm2\n\t"
        "vfixupimmss $0, %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovdqa %%xmm0, %0"
        : "=m"(out) : "m"(old_dst), "m"(source), "m"(control)
        : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(out.u32[0] == response_f32(12, old_dst.u32[0], source.u32[0]),
                "VFIXUPIMMSS response");
    for (int i = 1; i < 4; i++)
        TEST_ASSERT(out.u32[i] == source.u32[i],
                    "VFIXUPIMMSS upper source lane %d", i);

    uint32_t zero = 0;
    __asm__ volatile(
        "kmovb %1, %%k1\n\tvmovdqa %2, %%xmm0\n\tvmovdqa %3, %%xmm1\n\t"
        "vmovdqa %4, %%xmm2\n\t"
        "vfixupimmss $0, %%xmm2, %%xmm1, %%xmm0%{%%k1%}%{z%}\n\t"
        "vmovdqa %%xmm0, %0"
        : "=m"(out)
        : "r"(zero), "m"(old_dst), "m"(source), "m"(control)
        : "xmm0", "xmm1", "xmm2", "k1");
    TEST_ASSERT(out.u32[0] == 0, "VFIXUPIMMSS zero mask clears scalar element");
    for (int i = 1; i < 4; i++)
        TEST_ASSERT(out.u32[i] == source.u32[i],
                    "VFIXUPIMMSS zero mask keeps upper source lane %d", i);
}

static void test_scalar_double(void) {
    xmm_t old_dst, source, control, out;
    old_dst.u64[0] = UINT64_C(0x3ff0000000000000);
    old_dst.u64[1] = UINT64_C(0xaaaaaaaaaaaaaaaa);
    source.u64[0] = UINT64_C(0x4000000000000000);
    source.u64[1] = UINT64_C(0x1111222233334444);
    control.u64[0] = UINT32_C(0xdddddddd);

    __asm__ volatile(
        "vmovdqa %1, %%xmm0\n\tvmovdqa %2, %%xmm1\n\t"
        "vmovdqa %3, %%xmm2\n\t"
        "vfixupimmsd $0, %%xmm2, %%xmm1, %%xmm0\n\t"
        "vmovdqa %%xmm0, %0"
        : "=m"(out) : "m"(old_dst), "m"(source), "m"(control)
        : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(out.u64[0] == response_f64(13, old_dst.u64[0], source.u64[0]),
                "VFIXUPIMMSD response");
    TEST_ASSERT(out.u64[1] == source.u64[1],
                "VFIXUPIMMSD copies the upper source qword");

    uint32_t zero = 0;
    __asm__ volatile(
        "kmovb %1, %%k1\n\tvmovdqa %2, %%xmm0\n\tvmovdqa %3, %%xmm1\n\t"
        "vmovdqa %4, %%xmm2\n\t"
        "vfixupimmsd $0, %%xmm2, %%xmm1, %%xmm0%{%%k1%}\n\t"
        "vmovdqa %%xmm0, %0"
        : "=m"(out)
        : "r"(zero), "m"(old_dst), "m"(source), "m"(control)
        : "xmm0", "xmm1", "xmm2", "k1");
    TEST_ASSERT(out.u64[0] == old_dst.u64[0],
                "VFIXUPIMMSD merge mask keeps old scalar element");
    TEST_ASSERT(out.u64[1] == source.u64[1],
                "VFIXUPIMMSD merge mask copies the upper source qword");
}

int main(void) {
    TEST_START("AVX-512 VFIXUPIMM response and classification");
    if (!CHECK_AVX512F()) return 0;
    test_ps_response_table();
    test_ps_classification();
    test_pd_response_table();
    test_pd_classification();
    test_pd_masks();
    test_scalar_single();
    test_scalar_double();
    TEST_END();
}
