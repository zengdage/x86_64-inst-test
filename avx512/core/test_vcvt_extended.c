/* EVEX truncating, unsigned, and 64-bit integer floating conversions. */
#include "../include/avx512_test.h"
#include <math.h>

static void test_dword_conversions(void) {
    zmm_test_t src, nearest, truncated, unsigned_nearest, unsigned_truncated;
    const float values[16] = {
        0.0f, 0.5f, 1.5f, 2.5f, -0.5f, -1.5f, -2.5f, 3.75f,
        -3.75f, 42.9f, -42.9f, 16777215.0f, 16777216.0f,
        65535.5f, 2147483520.0f, -2147483648.0f
    };
    memcpy(src.f32, values, sizeof(values));
    __asm__ volatile(
        "vmovaps %4, %%zmm0\n\tvcvtps2dq %%zmm0, %%zmm1\n\t"
        "vcvttps2dq %%zmm0, %%zmm2\n\tvcvtps2udq %%zmm0, %%zmm3\n\t"
        "vcvttps2udq %%zmm0, %%zmm4\n\t"
        "vmovdqa32 %%zmm1, %0\n\tvmovdqa32 %%zmm2, %1\n\t"
        "vmovdqa32 %%zmm3, %2\n\tvmovdqa32 %%zmm4, %3"
        : "=m"(nearest), "=m"(truncated), "=m"(unsigned_nearest),
          "=m"(unsigned_truncated) : "m"(src)
        : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4");
    const int32_t nearest_expected[16] = {
        0, 0, 2, 2, 0, -2, -2, 4, -4, 43, -43,
        16777215, 16777216, 65536, 2147483520, INT32_MIN
    };
    const int32_t trunc_expected[16] = {
        0, 0, 1, 2, 0, -1, -2, 3, -3, 42, -42,
        16777215, 16777216, 65535, 2147483520, INT32_MIN
    };
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(nearest.i32[i] == nearest_expected[i], "VCVTPS2DQ lane %d", i);
        TEST_ASSERT(truncated.i32[i] == trunc_expected[i], "VCVTTPS2DQ lane %d", i);
    }
    for (int i = 0; i < 4; i++) {
        uint32_t expected_near = (uint32_t)nearbyintf(src.f32[i]);
        uint32_t expected_trunc = (uint32_t)src.f32[i];
        TEST_ASSERT(unsigned_nearest.u32[i] == expected_near, "VCVTPS2UDQ lane %d", i);
        TEST_ASSERT(unsigned_truncated.u32[i] == expected_trunc, "VCVTTPS2UDQ lane %d", i);
    }
    TEST_ASSERT(unsigned_nearest.u32[4] == 0 && unsigned_truncated.u32[4] == 0,
                "unsigned conversion accepts negative fractions that round/truncate to zero");
}

static void test_qword_conversions(void) {
    zmm_test_t integers, unsigned_integers, doubles, unsigned_doubles;
    const int64_t signed_values[8] = {
        INT64_MIN, INT64_MAX, -INT64_C(9007199254740993), -1,
        0, 1, INT64_C(9007199254740993), INT64_C(0x4000000000000000)
    };
    const uint64_t unsigned_values[8] = {
        0, 1, UINT64_C(9007199254740991), UINT64_C(9007199254740993),
        UINT64_C(0x7fffffffffffffff), UINT64_C(0x8000000000000000),
        UINT64_MAX - 1, UINT64_MAX
    };
    memcpy(integers.i64, signed_values, sizeof(signed_values));
    memcpy(unsigned_integers.u64, unsigned_values, sizeof(unsigned_values));
    __asm__ volatile(
        "vmovdqa64 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
        "vcvtqq2pd %%zmm0, %%zmm2\n\tvcvtuqq2pd %%zmm1, %%zmm3\n\t"
        "vmovapd %%zmm2, %0\n\tvmovapd %%zmm3, %1"
        : "=m"(doubles), "=m"(unsigned_doubles)
        : "m"(integers), "m"(unsigned_integers)
        : "zmm0", "zmm1", "zmm2", "zmm3");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(doubles.f64[i] == (double)signed_values[i], "VCVTQQ2PD lane %d", i);
        TEST_ASSERT(unsigned_doubles.f64[i] == (double)unsigned_values[i], "VCVTUQQ2PD lane %d", i);
    }

    zmm_test_t src, nearest, truncated;
    const double values[8] = {0.5, 1.5, 2.5, -0.5, -1.5, -2.5, 42.9, -42.9};
    memcpy(src.f64, values, sizeof(values));
    __asm__ volatile(
        "vmovapd %2, %%zmm0\n\tvcvtpd2qq %%zmm0, %%zmm1\n\t"
        "vcvttpd2qq %%zmm0, %%zmm2\n\t"
        "vmovdqa64 %%zmm1, %0\n\tvmovdqa64 %%zmm2, %1"
        : "=m"(nearest), "=m"(truncated) : "m"(src)
        : "zmm0", "zmm1", "zmm2");
    const int64_t nearest_expected[8] = {0, 2, 2, 0, -2, -2, 43, -43};
    const int64_t trunc_expected[8] = {0, 1, 2, 0, -1, -2, 42, -42};
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(nearest.i64[i] == nearest_expected[i], "VCVTPD2QQ lane %d", i);
        TEST_ASSERT(truncated.i64[i] == trunc_expected[i], "VCVTTPD2QQ lane %d", i);
    }

    const double unsigned_values_in[8] = {0.0, 0.5, 1.5, 2.5, 3.5, 42.9, 65535.5, 4294967295.0};
    memcpy(src.f64, unsigned_values_in, sizeof(unsigned_values_in));
    __asm__ volatile(
        "vmovapd %2, %%zmm0\n\tvcvtpd2uqq %%zmm0, %%zmm1\n\t"
        "vcvttpd2uqq %%zmm0, %%zmm2\n\t"
        "vmovdqa64 %%zmm1, %0\n\tvmovdqa64 %%zmm2, %1"
        : "=m"(nearest), "=m"(truncated) : "m"(src)
        : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(nearest.u64[i] == (uint64_t)nearbyint(src.f64[i]), "VCVTPD2UQQ lane %d", i);
        TEST_ASSERT(truncated.u64[i] == (uint64_t)src.f64[i], "VCVTTPD2UQQ lane %d", i);
    }
}

int main(void) {
    TEST_START("Extended EVEX floating/integer conversions");
    if (!CHECK_AVX512F()) return 0;
    test_dword_conversions();
    test_qword_conversions();
    TEST_END();
}
