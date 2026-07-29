/*
 * test_vperm2f128.c - Test VPERM2F128/VPERM2I128 instructions (AVX)
 *
 * VPERM2F128: Permute two 128-bit lanes from two 256-bit sources (float).
 * VPERM2I128: Same for integer data (AVX2).
 * imm8 bits [3:0] select dst low lane, [7:4] select dst high lane.
 * Each 4-bit field: bits[1:0]=lane selector (0-3), bit[3]=zero if set.
 *
 * Compile: gcc -o test_vperm2f128 simd/test_vperm2f128.c -O0 -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vperm2f128_identity(void) {
    ymm_t a = { .f32 = {1,2,3,4, 5,6,7,8} };
    ymm_t b = { .f32 = {10,20,30,40, 50,60,70,80} };
    ymm_t dst;

    /* 0x20: low=a_low(0), high=b_low(2) */
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vperm2f128 $0x20, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.f32[0] == 1.0f, "vperm2f128 $20 low[0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[4] == 10.0f, "vperm2f128 $20 high[4]: got %f", dst.f32[4]);
}

static void test_vperm2f128_swap(void) {
    ymm_t a = { .f32 = {1,2,3,4, 5,6,7,8} };
    ymm_t dst;

    /* 0x01: low=a_high(1), high=a_low(0) */
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vperm2f128 $0x01, %%ymm0, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.f32[0] == 5.0f, "vperm2f128 swap [0]: got %f", dst.f32[0]);
    TEST_ASSERT(dst.f32[4] == 1.0f, "vperm2f128 swap [4]: got %f", dst.f32[4]);
}

static void test_vperm2f128_zero_lane(void) {
    ymm_t a = { .f32 = {1,2,3,4, 5,6,7,8} };
    ymm_t dst;

    /* 0x80: low=zeroed (bit3 set), high=a_low(0) */
    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vperm2f128 $0x08, %%ymm0, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a) : "ymm0", "ymm1"
    );
    /* imm8=0x08: low nibble=8 (bit3 set => zero), high nibble=0 (a_low) */
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == 0.0f,
            "vperm2f128 zero lane [%d]: got %f", i, dst.f32[i]);
    }
    TEST_ASSERT(dst.f32[4] == 1.0f, "vperm2f128 zero lane [4]: got %f", dst.f32[4]);
}

static void test_vperm2i128_basic(void) {
    ymm_t a = { .i32 = {1,2,3,4, 5,6,7,8} };
    ymm_t b = { .i32 = {10,20,30,40, 50,60,70,80} };
    ymm_t dst;

    /* 0x31: low=a_high(1), high=b_high(3) */
    __asm__ volatile (
        "vmovdqa %1, %%ymm0\n\t"
        "vperm2i128 $0x31, %2, %%ymm0, %%ymm1\n\t"
        "vmovdqa %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.i32[0] == 5, "vperm2i128 $31 low[0]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[4] == 50, "vperm2i128 $31 high[4]: got %d", dst.i32[4]);
}

int main(void) {
    TEST_START("VPERM2F128/VPERM2I128 instructions (AVX)");
    test_vperm2f128_identity();
    test_vperm2f128_swap();
    test_vperm2f128_zero_lane();
    test_vperm2i128_basic();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
