/*
 * TARGETS: CVTDQ2PD CVTDQ2PS CVTPD2DQ CVTPS2DQ CVTPS2PD CVTSD2SI
 * CVTSD2SS CVTSI2SD CVTSI2SS CVTSS2SD CVTSS2SI CVTTPD2DQ
 * CVTTPS2DQ CVTTSD2SI CVTTSS2SI CVTPI2PS CVTPS2PI CVTTPS2PI
 * CVTPI2PD CVTPD2PI CVTTPD2PI
 */
#include "../common.h"

static void test_packed(void) {
    xmm_t out;
    const xmm_t ints = { .i32 = {INT32_MIN, -3, 7, INT32_MAX} };
    const xmm_t floats = { .f32 = {1.4f, 2.6f, -3.4f, -4.6f} };
    const xmm_t doubles = { .f64 = {2.6, -3.4} };

    __asm__ volatile("cvtdq2pd %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(ints) : "xmm0");
    TEST_ASSERT(out.f64[0] == (double)INT32_MIN, "CVTDQ2PD lane 0");
    TEST_ASSERT(out.f64[1] == -3.0, "CVTDQ2PD lane 1");

    __asm__ volatile("cvtdq2ps %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(ints) : "xmm0");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.f32[i] == (float)ints.i32[i], "CVTDQ2PS lane %d", i);

    __asm__ volatile("cvtpd2dq %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(doubles) : "xmm0");
    TEST_ASSERT(out.i32[0] == 3 && out.i32[1] == -3, "CVTPD2DQ values");
    TEST_ASSERT(out.u64[1] == 0, "CVTPD2DQ clears high qword");

    __asm__ volatile("cvtps2dq %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(floats) : "xmm0");
    const int32_t rounded[4] = {1, 3, -3, -5};
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.i32[i] == rounded[i], "CVTPS2DQ lane %d", i);

    __asm__ volatile("cvttps2dq %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(floats) : "xmm0");
    const int32_t truncated[4] = {1, 2, -3, -4};
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.i32[i] == truncated[i], "CVTTPS2DQ lane %d", i);

    __asm__ volatile("cvtps2pd %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(floats) : "xmm0");
    TEST_ASSERT(out.f64[0] == (double)floats.f32[0], "CVTPS2PD lane 0");
    TEST_ASSERT(out.f64[1] == (double)floats.f32[1], "CVTPS2PD lane 1");

    __asm__ volatile("cvttpd2dq %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(doubles) : "xmm0");
    TEST_ASSERT(out.i32[0] == 2 && out.i32[1] == -3, "CVTTPD2DQ values");
    TEST_ASSERT(out.u64[1] == 0, "CVTTPD2DQ clears high qword");
}

static void test_scalar(void) {
    const xmm_t preserved = { .u32 = {
        UINT32_C(0x3f800000), UINT32_C(0x11223344),
        UINT32_C(0x55667788), UINT32_C(0x99aabbcc)
    } };
    const float fs = -3.75f;
    const double ds = 6.75;
    const int64_t integer = INT64_C(-9007199254740991);
    xmm_t out;
    int64_t i64;

    __asm__ volatile("cvttss2si %1, %0" : "=r"(i64) : "m"(fs));
    TEST_ASSERT(i64 == -3, "CVTTSS2SI 64-bit result");
    __asm__ volatile("cvtss2si %1, %0" : "=r"(i64) : "m"(fs));
    TEST_ASSERT(i64 == -4, "CVTSS2SI 64-bit rounded result");
    __asm__ volatile("cvttsd2si %1, %0" : "=r"(i64) : "m"(ds));
    TEST_ASSERT(i64 == 6, "CVTTSD2SI 64-bit result");
    __asm__ volatile("cvtsd2si %1, %0" : "=r"(i64) : "m"(ds));
    TEST_ASSERT(i64 == 7, "CVTSD2SI 64-bit rounded result");

    __asm__ volatile("movdqu %1, %%xmm0\n\tcvtsi2ssq %2, %%xmm0\n\t"
                     "movdqu %%xmm0, %0"
                     : "=m"(out) : "m"(preserved), "r"(integer) : "xmm0");
    TEST_ASSERT(out.f32[0] == (float)integer, "CVTSI2SS result");
    for (int i = 1; i < 4; i++)
        TEST_ASSERT(out.u32[i] == preserved.u32[i], "CVTSI2SS preserves lane %d", i);

    __asm__ volatile("movdqu %1, %%xmm0\n\tcvtsi2sdq %2, %%xmm0\n\t"
                     "movdqu %%xmm0, %0"
                     : "=m"(out) : "m"(preserved), "r"(integer) : "xmm0");
    TEST_ASSERT(out.f64[0] == (double)integer, "CVTSI2SD result");
    TEST_ASSERT(out.u64[1] == preserved.u64[1], "CVTSI2SD preserves high qword");

    __asm__ volatile("movdqu %1, %%xmm0\n\tcvtss2sd %2, %%xmm0\n\t"
                     "movdqu %%xmm0, %0"
                     : "=m"(out) : "m"(preserved), "m"(fs) : "xmm0");
    TEST_ASSERT(out.f64[0] == (double)fs, "CVTSS2SD result");
    TEST_ASSERT(out.u64[1] == preserved.u64[1], "CVTSS2SD preserves high qword");

    __asm__ volatile("movdqu %1, %%xmm0\n\tcvtsd2ss %2, %%xmm0\n\t"
                     "movdqu %%xmm0, %0"
                     : "=m"(out) : "m"(preserved), "m"(ds) : "xmm0");
    TEST_ASSERT(out.f32[0] == (float)ds, "CVTSD2SS result");
    for (int i = 1; i < 4; i++)
        TEST_ASSERT(out.u32[i] == preserved.u32[i], "CVTSD2SS preserves lane %d", i);
}

static void test_scalar_32bit_integer_forms(void) {
    const xmm_t preserved = { .u32 = {
        UINT32_C(0xdeadbeef), UINT32_C(0x11223344),
        UINT32_C(0x55667788), UINT32_C(0x99aabbcc)
    } };
    const int32_t reg_integer = INT32_C(-16777217);
    const int32_t mem_integer = INT32_MAX;
    xmm_t out;

    __asm__ volatile (
        "movdqu %[preserved], %%xmm0\n\tcvtsi2ssl %[integer], %%xmm0\n\t"
        "movdqu %%xmm0, %[out]"
        : [out] "=m"(out)
        : [preserved] "m"(preserved), [integer] "r"(reg_integer) : "xmm0");
    TEST_ASSERT(out.f32[0] == (float)reg_integer, "CVTSI2SS r32 result");
    for (int i = 1; i < 4; ++i)
        TEST_ASSERT(out.u32[i] == preserved.u32[i], "CVTSI2SS r32 preserves lane %d", i);

    __asm__ volatile (
        "movdqu %[preserved], %%xmm0\n\tcvtsi2ssl %[integer], %%xmm0\n\t"
        "movdqu %%xmm0, %[out]"
        : [out] "=m"(out)
        : [preserved] "m"(preserved), [integer] "m"(mem_integer) : "xmm0");
    TEST_ASSERT(out.f32[0] == (float)mem_integer, "CVTSI2SS m32 result");

    __asm__ volatile (
        "movdqu %[preserved], %%xmm0\n\tcvtsi2sdl %[integer], %%xmm0\n\t"
        "movdqu %%xmm0, %[out]"
        : [out] "=m"(out)
        : [preserved] "m"(preserved), [integer] "r"(reg_integer) : "xmm0");
    TEST_ASSERT(out.f64[0] == (double)reg_integer, "CVTSI2SD r32 result");
    TEST_ASSERT(out.u64[1] == preserved.u64[1], "CVTSI2SD r32 preserves high qword");

    __asm__ volatile (
        "movdqu %[preserved], %%xmm0\n\tcvtsi2sdl %[integer], %%xmm0\n\t"
        "movdqu %%xmm0, %[out]"
        : [out] "=m"(out)
        : [preserved] "m"(preserved), [integer] "m"(mem_integer) : "xmm0");
    TEST_ASSERT(out.f64[0] == (double)mem_integer, "CVTSI2SD m32 result");

    const float fs = -3.75f;
    const double ds = 6.75;
    int32_t rounded, truncated;
    __asm__ volatile ("cvtss2si %2, %0\n\tcvttss2si %2, %1"
                      : "=r"(rounded), "=r"(truncated) : "m"(fs));
    TEST_ASSERT(rounded == -4, "CVTSS2SI r32 rounds according to MXCSR");
    TEST_ASSERT(truncated == -3, "CVTTSS2SI r32 truncates");
    __asm__ volatile ("cvtsd2si %2, %0\n\tcvttsd2si %2, %1"
                      : "=r"(rounded), "=r"(truncated) : "m"(ds));
    TEST_ASSERT(rounded == 7, "CVTSD2SI r32 rounds according to MXCSR");
    TEST_ASSERT(truncated == 6, "CVTTSD2SI r32 truncates");

    const uint32_t qnan_bits = UINT32_C(0x7fc12345);
    __asm__ volatile ("cvtss2si %1, %0" : "=r"(rounded) : "m"(qnan_bits));
    TEST_ASSERT(rounded == INT32_MIN, "CVTSS2SI r32 QNaN produces integer indefinite");
}

static void test_scalar_register_sources_and_m64(void) {
    const float fs = -3.75f;
    const double ds = 6.75;
    int32_t out32, trunc32;
    int64_t out64, trunc64;
    __asm__ volatile (
        "movss %[source], %%xmm0\n\t"
        "cvtss2si %%xmm0, %[rounded32]\n\t"
        "cvttss2si %%xmm0, %[truncated32]\n\t"
        "cvtss2si %%xmm0, %[rounded64]\n\t"
        "cvttss2si %%xmm0, %[truncated64]"
        : [rounded32] "=r"(out32), [truncated32] "=r"(trunc32),
          [rounded64] "=r"(out64), [truncated64] "=r"(trunc64)
        : [source] "m"(fs) : "xmm0");
    TEST_ASSERT(out32 == -4 && out64 == -4,
                "CVTSS2SI XMM source rounds to r32/r64");
    TEST_ASSERT(trunc32 == -3 && trunc64 == -3,
                "CVTTSS2SI XMM source truncates to r32/r64");

    __asm__ volatile (
        "movsd %[source], %%xmm0\n\t"
        "cvtsd2si %%xmm0, %[out32]\n\t"
        "cvtsd2si %%xmm0, %[out64]"
        : [out32] "=r"(out32), [out64] "=r"(out64)
        : [source] "m"(ds) : "xmm0");
    TEST_ASSERT(out32 == 7 && out64 == 7,
                "CVTSD2SI XMM source rounds to r32/r64");
    __asm__ volatile (
        "movsd %[source], %%xmm0\n\t"
        "cvttsd2si %%xmm0, %[out32]\n\t"
        "cvttsd2si %%xmm0, %[out64]"
        : [out32] "=r"(trunc32), [out64] "=r"(trunc64)
        : [source] "m"(ds) : "xmm0");
    TEST_ASSERT(trunc32 == 6 && trunc64 == 6,
                "CVTTSD2SI XMM source truncates to r32/r64");

    const xmm_t preserved = { .u64 = {
        UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210)} };
    const int64_t integer = INT64_C(-9007199254740991);
    xmm_t result;
    __asm__ volatile (
        "movdqu %[preserved], %%xmm0\n\tcvtsi2ssq %[integer], %%xmm0\n\t"
        "movdqu %%xmm0, %[result]"
        : [result] "=m"(result)
        : [preserved] "m"(preserved), [integer] "m"(integer) : "xmm0");
    TEST_ASSERT(result.f32[0] == (float)integer, "CVTSI2SS m64 result");
    TEST_ASSERT(result.u32[1] == preserved.u32[1] &&
                result.u64[1] == preserved.u64[1], "CVTSI2SS m64 preserves upper lanes");
    __asm__ volatile (
        "movdqu %[preserved], %%xmm0\n\tcvtsi2sdq %[integer], %%xmm0\n\t"
        "movdqu %%xmm0, %[result]"
        : [result] "=m"(result)
        : [preserved] "m"(preserved), [integer] "m"(integer) : "xmm0");
    TEST_ASSERT(result.f64[0] == (double)integer, "CVTSI2SD m64 result");
    TEST_ASSERT(result.u64[1] == preserved.u64[1], "CVTSI2SD m64 preserves high qword");
}

static void test_mmx_conversions(void) {
    const uint64_t packed_i32 = UINT64_C(0xfffffffd00000007);
    const xmm_t ps = { .f32 = {2.6f, -3.4f, 99.0f, 100.0f} };
    const xmm_t pd = { .f64 = {2.6, -3.4} };
    xmm_t out;
    uint64_t mm;

    __asm__ volatile("movq %1, %%mm0\n\tcvtpi2ps %%mm0, %%xmm0\n\t"
                     "movdqu %%xmm0, %0\n\temms"
                     : "=m"(out) : "m"(packed_i32) : "mm0", "xmm0");
    TEST_ASSERT(out.f32[0] == 7.0f && out.f32[1] == -3.0f, "CVTPI2PS values");

    __asm__ volatile("cvtps2pi %1, %%mm0\n\tmovq %%mm0, %0\n\temms"
                     : "=m"(mm) : "m"(ps) : "mm0");
    TEST_ASSERT((int32_t)mm == 3 && (int32_t)(mm >> 32) == -3, "CVTPS2PI values");
    __asm__ volatile("cvttps2pi %1, %%mm0\n\tmovq %%mm0, %0\n\temms"
                     : "=m"(mm) : "m"(ps) : "mm0");
    TEST_ASSERT((int32_t)mm == 2 && (int32_t)(mm >> 32) == -3, "CVTTPS2PI values");

    __asm__ volatile("movq %1, %%mm0\n\tcvtpi2pd %%mm0, %%xmm0\n\t"
                     "movdqu %%xmm0, %0\n\temms"
                     : "=m"(out) : "m"(packed_i32) : "mm0", "xmm0");
    TEST_ASSERT(out.f64[0] == 7.0 && out.f64[1] == -3.0, "CVTPI2PD values");
    __asm__ volatile("cvtpd2pi %1, %%mm0\n\tmovq %%mm0, %0\n\temms"
                     : "=m"(mm) : "m"(pd) : "mm0");
    TEST_ASSERT((int32_t)mm == 3 && (int32_t)(mm >> 32) == -3, "CVTPD2PI values");
    __asm__ volatile("cvttpd2pi %1, %%mm0\n\tmovq %%mm0, %0\n\temms"
                     : "=m"(mm) : "m"(pd) : "mm0");
    TEST_ASSERT((int32_t)mm == 2 && (int32_t)(mm >> 32) == -3, "CVTTPD2PI values");
}

int main(void) {
    TEST_START("SSE/SSE2 conversion gaps");
    test_packed();
    test_scalar();
    test_scalar_32bit_integer_forms();
    test_scalar_register_sources_and_m64();
    test_mmx_conversions();
    TEST_END();
}
