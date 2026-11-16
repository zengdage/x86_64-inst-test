/* AVX-512F/DQ packed conversion forms missing from the wider conversion
 * tests: unsigned dword to float/double, qword to float, float to qword,
 * and double to unsigned dword.
 */
#include "../include/avx512_test.h"
#include <limits.h>
#include <math.h>

static void test_unsigned_dword_to_fp(void) {
    static const uint32_t values[16] = {
        0, 1, 2, 3, UINT32_C(0x00ffffff), UINT32_C(0x01000000),
        UINT32_C(0x01000001), UINT32_C(0x7fffffff), UINT32_C(0x80000000),
        UINT32_C(0x80000001), UINT32_C(0xffffff00), UINT32_MAX,
        16, 256, 65535, UINT32_C(0x40000000)
    };
    zmm_test_t src, single, doubles;
    memcpy(src.u32, values, sizeof(values));

    __asm__ volatile(
        "vmovdqa32 %2, %%zmm0\n\t"
        "vcvtudq2ps %%zmm0, %%zmm1\n\t"
        "vcvtudq2pd %%ymm0, %%zmm2\n\t"
        "vmovaps %%zmm1, %0\n\t"
        "vmovapd %%zmm2, %1"
        : "=m"(single), "=m"(doubles) : "m"(src)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(single.f32[i] == (float)values[i],
                    "VCVTUDQ2PS lane %d: %a", i, single.f32[i]);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(doubles.f64[i] == (double)values[i],
                    "VCVTUDQ2PD lane %d: %a", i, doubles.f64[i]);

    TEST_ASSERT(single.f32[4] == 0x1.fffffep+23f,
                "VCVTUDQ2PS keeps 2^24-1 exactly");
    TEST_ASSERT(single.f32[6] == 0x1p+24f,
                "VCVTUDQ2PS rounds 2^24+1 to even");
    TEST_ASSERT(single.f32[11] == 0x1p+32f,
                "VCVTUDQ2PS rounds UINT32_MAX to 2^32");
}

static void test_qword_to_single(void) {
    static const int64_t signed_values[8] = {
        INT64_MIN, -INT64_C(16777217), -1, 0, 1,
        INT64_C(16777217), INT64_C(0x4000000000000000), INT64_MAX
    };
    static const uint64_t unsigned_values[8] = {
        0, 1, UINT64_C(16777215), UINT64_C(16777217),
        UINT64_C(0x7fffffffffffffff), UINT64_C(0x8000000000000000),
        UINT64_MAX - 1, UINT64_MAX
    };
    zmm_test_t signed_src, unsigned_src;
    ymm_t signed_out, unsigned_out;
    memcpy(signed_src.i64, signed_values, sizeof(signed_values));
    memcpy(unsigned_src.u64, unsigned_values, sizeof(unsigned_values));

    __asm__ volatile(
        "vmovdqa64 %2, %%zmm0\n\t"
        "vmovdqa64 %3, %%zmm1\n\t"
        "vcvtqq2ps %%zmm0, %%ymm2\n\t"
        "vcvtuqq2ps %%zmm1, %%ymm3\n\t"
        "vmovaps %%ymm2, %0\n\t"
        "vmovaps %%ymm3, %1"
        : "=m"(signed_out), "=m"(unsigned_out)
        : "m"(signed_src), "m"(unsigned_src)
        : "zmm0", "zmm1", "ymm2", "ymm3");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(signed_out.f32[i] == (float)signed_values[i],
                    "VCVTQQ2PS lane %d: %a", i, signed_out.f32[i]);
        TEST_ASSERT(unsigned_out.f32[i] == (float)unsigned_values[i],
                    "VCVTUQQ2PS lane %d: %a", i, unsigned_out.f32[i]);
    }
    TEST_ASSERT(signed_out.f32[1] == -0x1p+24f && signed_out.f32[5] == 0x1p+24f,
                "VCVTQQ2PS rounds signed 2^24+1 ties toward even");
    TEST_ASSERT(unsigned_out.f32[7] == 0x1p+64f,
                "VCVTUQQ2PS rounds UINT64_MAX to 2^64");
}

static void test_single_to_signed_qword(void) {
    static const float values[8] = {
        0.5f, 1.5f, 2.5f, -0.5f, -1.5f, -2.5f, 42.9f, -42.9f
    };
    static const int64_t nearest_expected[8] = {0, 2, 2, 0, -2, -2, 43, -43};
    static const int64_t trunc_expected[8] = {0, 1, 2, 0, -1, -2, 42, -42};
    ymm_t src;
    zmm_test_t nearest, truncated;
    memcpy(src.f32, values, sizeof(values));

    __asm__ volatile(
        "vmovaps %2, %%ymm0\n\t"
        "vcvtps2qq %%ymm0, %%zmm1\n\t"
        "vcvttps2qq %%ymm0, %%zmm2\n\t"
        "vmovdqa64 %%zmm1, %0\n\t"
        "vmovdqa64 %%zmm2, %1"
        : "=m"(nearest), "=m"(truncated) : "m"(src)
        : "ymm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(nearest.i64[i] == nearest_expected[i],
                    "VCVTPS2QQ lane %d", i);
        TEST_ASSERT(truncated.i64[i] == trunc_expected[i],
                    "VCVTTPS2QQ lane %d", i);
    }
}

static void test_single_to_unsigned_qword(void) {
    static const float values[8] = {
        0.0f, 0.5f, 1.5f, 2.5f, 3.5f, 42.9f, 0x1p+63f, -1.0f
    };
    static const uint64_t nearest_expected[8] = {
        0, 0, 2, 2, 4, 43, UINT64_C(0x8000000000000000), UINT64_MAX
    };
    static const uint64_t trunc_expected[8] = {
        0, 0, 1, 2, 3, 42, UINT64_C(0x8000000000000000), UINT64_MAX
    };
    ymm_t src;
    zmm_test_t nearest, truncated;
    memcpy(src.f32, values, sizeof(values));

    __asm__ volatile(
        "vmovaps %2, %%ymm0\n\t"
        "vcvtps2uqq %%ymm0, %%zmm1\n\t"
        "vcvttps2uqq %%ymm0, %%zmm2\n\t"
        "vmovdqa64 %%zmm1, %0\n\t"
        "vmovdqa64 %%zmm2, %1"
        : "=m"(nearest), "=m"(truncated) : "m"(src)
        : "ymm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(nearest.u64[i] == nearest_expected[i],
                    "VCVTPS2UQQ lane %d", i);
        TEST_ASSERT(truncated.u64[i] == trunc_expected[i],
                    "VCVTTPS2UQQ lane %d", i);
    }
}

static void test_double_to_unsigned_dword(void) {
    static const double values[8] = {
        0.0, 0.5, 1.5, 2.5, 3.5, 42.9, 4294967295.0, -1.0
    };
    static const uint32_t nearest_expected[8] = {
        0, 0, 2, 2, 4, 43, UINT32_MAX, UINT32_MAX
    };
    static const uint32_t trunc_expected[8] = {
        0, 0, 1, 2, 3, 42, UINT32_MAX, UINT32_MAX
    };
    zmm_test_t src;
    ymm_t nearest, truncated;
    memcpy(src.f64, values, sizeof(values));

    __asm__ volatile(
        "vmovapd %2, %%zmm0\n\t"
        "vcvtpd2udq %%zmm0, %%ymm1\n\t"
        "vcvttpd2udq %%zmm0, %%ymm2\n\t"
        "vmovdqa32 %%ymm1, %0\n\t"
        "vmovdqa32 %%ymm2, %1"
        : "=m"(nearest), "=m"(truncated) : "m"(src)
        : "zmm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(nearest.u32[i] == nearest_expected[i],
                    "VCVTPD2UDQ lane %d", i);
        TEST_ASSERT(truncated.u32[i] == trunc_expected[i],
                    "VCVTTPD2UDQ lane %d", i);
    }
}

static void test_masking(void) {
    ymm_t src;
    zmm_test_t initial, merged, zeroed;
    for (int i = 0; i < 8; i++) {
        src.f32[i] = (float)(i + 1) + 0.75f;
        initial.u64[i] = UINT64_C(0xdeadbeef00000000) + (uint64_t)i;
    }
    const uint32_t mask = UINT32_C(0x81);
    __asm__ volatile(
        "kmovb %3, %%k1\n\t"
        "vmovaps %4, %%ymm0\n\t"
        "vmovdqa64 %2, %%zmm1\n\t"
        "vcvttps2qq %%ymm0, %%zmm1%{%%k1%}\n\t"
        "vcvttps2qq %%ymm0, %%zmm2%{%%k1%}%{z%}\n\t"
        "vmovdqa64 %%zmm1, %0\n\t"
        "vmovdqa64 %%zmm2, %1"
        : "=m"(merged), "=m"(zeroed)
        : "m"(initial), "r"(mask), "m"(src)
        : "ymm0", "zmm1", "zmm2", "k1");
    for (int i = 0; i < 8; i++) {
        uint64_t converted = (uint64_t)(i + 1);
        TEST_ASSERT(merged.u64[i] == ((mask >> i) & 1U ? converted : initial.u64[i]),
                    "VCVTTPS2QQ merge mask lane %d", i);
        TEST_ASSERT(zeroed.u64[i] == ((mask >> i) & 1U ? converted : 0),
                    "VCVTTPS2QQ zero mask lane %d", i);
    }
}

int main(void) {
    TEST_START("AVX-512 packed conversion gaps");
    if (!CHECK_AVX512F() || !CHECK_AVX512DQ()) return 0;
    test_unsigned_dword_to_fp();
    test_qword_to_single();
    test_single_to_signed_qword();
    test_single_to_unsigned_qword();
    test_double_to_unsigned_dword();
    test_masking();
    TEST_END();
}
