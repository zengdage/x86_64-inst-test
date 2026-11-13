/* AVX-512VNNI byte/word dot products and saturating variants. */
#include "avx512_test.h"
#include <limits.h>

static int32_t clamp_i64(int64_t value) {
    if (value > INT32_MAX) return INT32_MAX;
    if (value < INT32_MIN) return INT32_MIN;
    return (int32_t)value;
}

static void test_byte_dot_products(void) {
    zmm_test_t acc, ubytes, sbytes, normal, saturated;
    for (int i = 0; i < 16; i++) acc.i32[i] = i * 100 - 700;
    for (int i = 0; i < 64; i++) {
        ubytes.u8[i] = (uint8_t)(i * 13U + 1U);
        sbytes.i8[i] = (int8_t)(i * 7 - 100);
    }
    __asm__ volatile("vmovdqa32 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
                     "vmovdqa64 %4, %%zmm2\n\t"
                     "vpdpbusd %%zmm2, %%zmm1, %%zmm0\n\tvmovdqa32 %%zmm0, %0\n\t"
                     "vmovdqa32 %2, %%zmm0\n\tvpdpbusds %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %1"
                     : "=m"(normal), "=m"(saturated)
                     : "m"(acc), "m"(ubytes), "m"(sbytes)
                     : "zmm0", "zmm1", "zmm2");
    for (int lane = 0; lane < 16; lane++) {
        int64_t sum = acc.i32[lane];
        for (int j = 0; j < 4; j++)
            sum += (int64_t)ubytes.u8[lane * 4 + j] * sbytes.i8[lane * 4 + j];
        TEST_ASSERT(normal.u32[lane] == (uint32_t)sum, "VPDPBUSD lane %d", lane);
        TEST_ASSERT(saturated.i32[lane] == clamp_i64(sum), "VPDPBUSDS lane %d", lane);
    }
}

static void test_word_dot_products(void) {
    zmm_test_t acc, a, b, normal, saturated;
    for (int i = 0; i < 16; i++) acc.i32[i] = i - 8;
    for (int i = 0; i < 32; i++) {
        a.i16[i] = (int16_t)(i * 997 - 15000);
        b.i16[i] = (int16_t)(20000 - i * 733);
    }
    __asm__ volatile("vmovdqa32 %2, %%zmm0\n\tvmovdqa64 %3, %%zmm1\n\t"
                     "vmovdqa64 %4, %%zmm2\n\t"
                     "vpdpwssd %%zmm2, %%zmm1, %%zmm0\n\tvmovdqa32 %%zmm0, %0\n\t"
                     "vmovdqa32 %2, %%zmm0\n\tvpdpwssds %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %1"
                     : "=m"(normal), "=m"(saturated)
                     : "m"(acc), "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2");
    for (int lane = 0; lane < 16; lane++) {
        int64_t sum = acc.i32[lane];
        for (int j = 0; j < 2; j++)
            sum += (int64_t)a.i16[lane * 2 + j] * b.i16[lane * 2 + j];
        TEST_ASSERT(normal.u32[lane] == (uint32_t)sum, "VPDPWSSD lane %d", lane);
        TEST_ASSERT(saturated.i32[lane] == clamp_i64(sum), "VPDPWSSDS lane %d", lane);
    }

    for (int i = 0; i < 16; i++) acc.i32[i] = INT32_MAX - 1;
    for (int i = 0; i < 32; i++) a.i16[i] = b.i16[i] = INT16_MAX;
    __asm__ volatile("vmovdqa32 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
                     "vmovdqa64 %3, %%zmm2\n\tvpdpwssds %%zmm2, %%zmm1, %%zmm0\n\t"
                     "vmovdqa32 %%zmm0, %0"
                     : "=m"(saturated) : "m"(acc), "m"(a), "m"(b)
                     : "zmm0", "zmm1", "zmm2");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(saturated.i32[i] == INT32_MAX, "VPDPWSSDS positive saturation lane %d", i);
}

int main(void) {
    TEST_START("AVX-512VNNI dot products");
    if (!CHECK_AVX512VNNI()) return 0;
    test_byte_dot_products();
    test_word_dot_products();
    TEST_END();
}
