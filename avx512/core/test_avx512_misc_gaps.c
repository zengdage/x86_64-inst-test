/* AVX-512 forms that the wider suite does not cover: the qword gather and
 * scatter with dword indices, the 256-bit extraction forms and the EVEX
 * scalar integer conversions.
 */
#include "../include/avx512_test.h"
#include <math.h>

static void test_scatter_qword_512(void) {
    int64_t data[64];
    zmm_test_t index, values;
    for (int i = 0; i < 64; i++) data[i] = -1;
    for (int i = 0; i < 8; i++) index.u64[i] = (uint64_t)(i * 4);
    for (int i = 0; i < 8; i++) values.u64[i] = UINT64_C(0xbeef0000) + (uint64_t)i;

    uint32_t mask = UINT32_C(0x0f0f);
    __asm__ volatile("kmovd %2, %%k1\n\t"
                     "vmovdqa32 %1, %%zmm0\n\t"
                     "vmovdqu64 %3, %%zmm1\n\t"
                     "vpscatterqq %%zmm1, (%0,%%zmm0,8)%{%%k1%}"
                     : : "r"(data), "m"(index), "r"(mask), "m"(values)
                     : "zmm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 8; i++) {
        int slot = (int)index.u64[i];
        if ((mask >> i) & 1U)
            TEST_ASSERT(data[slot] == (int64_t)values.u64[i],
                        "VPSCATTERQQ lane %d", i);
        else
            TEST_ASSERT(data[slot] == -1, "VPSCATTERQQ masked lane %d", i);
    }
}

static void test_extract_256(void) {
    zmm_test_t src, out;
    for (int i = 0; i < 16; i++) src.u32[i] = UINT32_C(0x11223300) + (uint32_t)i;
    __asm__ volatile("kmovd %2, %%k1\n\t"
                     "vmovdqu32 %1, %%zmm0\n\t"
                     "vextracti32x8 $0x1, %%zmm0, %%ymm1%{%%k1%}\n\t"
                     "vmovdqu32 %%ymm1, %0"
                     : "=m"(out) : "m"(src), "r"(UINT32_C(0xff)) : "zmm0", "ymm1", "k1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == src.u32[8 + i], "VEXTRACTI32X8 lane %d", i);

    for (int i = 0; i < 8; i++) src.u64[i] = UINT64_C(0x1122334455667788) + (uint64_t)i;
    __asm__ volatile("kmovd %2, %%k1\n\t"
                     "vmovdqu64 %1, %%zmm0\n\t"
                     "vextracti64x4 $0x1, %%zmm0, %%ymm1%{%%k1%}\n\t"
                     "vmovdqu64 %%ymm1, %0"
                     : "=m"(out) : "m"(src), "r"(UINT32_C(0xf)) : "zmm0", "ymm1", "k1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == src.u64[4 + i], "VEXTRACTI64X4 lane %d", i);
}

static void test_scalar_conversions(void) {
    double value = 42.75;
    uint64_t unsigned_result = 0;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvcvttsd2usi %%xmm0, %%rax\n\t"
                     "movq %%rax, %0"
                     : "=r"(unsigned_result) : "m"(value) : "xmm0", "rax", "cc");
    TEST_ASSERT(unsigned_result == 42, "VCVTTSD2USI truncates toward zero: %llu",
                (unsigned long long)unsigned_result);

    double negative = -1.0;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvcvttsd2usi %%xmm0, %%rax\n\t"
                     "movq %%rax, %0"
                     : "=r"(unsigned_result) : "m"(negative) : "xmm0", "rax", "cc");
    TEST_ASSERT(unsigned_result == UINT64_MAX,
                "VCVTTSD2USI out of range is the unsigned integer indefinite value");

    float single = 7.9f;
    __asm__ volatile("vmovss %1, %%xmm0\n\tvcvttss2usi %%xmm0, %%rax\n\t"
                     "movq %%rax, %0"
                     : "=r"(unsigned_result) : "m"(single) : "xmm0", "rax", "cc");
    TEST_ASSERT(unsigned_result == 7, "VCVTTSS2USI truncates toward zero: %llu",
                (unsigned long long)unsigned_result);

    double whole = 1234.0;
    int64_t signed_result = 0;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvcvttsd2si %%xmm0, %%rax\n\t"
                     "movq %%rax, %0"
                     : "=r"(signed_result) : "m"(whole) : "xmm0", "rax", "cc");
    TEST_ASSERT(signed_result == 1234, "VCVTTSD2SI truncates: %lld", (long long)signed_result);

    double truncated = 12.75;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvcvttsd2si %%xmm0, %%rax\n\t"
                     "movq %%rax, %0"
                     : "=r"(signed_result) : "m"(truncated) : "xmm0", "rax", "cc");
    TEST_ASSERT(signed_result == 12, "VCVTTSD2SI truncates toward zero: %lld",
                (long long)signed_result);

    float single_truncated = -3.5f;
    __asm__ volatile("vmovss %1, %%xmm0\n\tvcvttss2si %%xmm0, %%rax\n\t"
                     "movq %%rax, %0"
                     : "=r"(signed_result) : "m"(single_truncated) : "xmm0", "rax", "cc");
    TEST_ASSERT(signed_result == -3, "VCVTTSS2SI truncates toward zero: %lld",
                (long long)signed_result);
}

int main(void) {
    TEST_START("AVX-512 miscellaneous gap instructions");
    if (!CHECK_AVX512F() || !CHECK_AVX512DQ() || !CHECK_AVX512VL()) return 0;
    test_scatter_qword_512();
    test_extract_256();
    test_scalar_conversions();
    TEST_END();
}
