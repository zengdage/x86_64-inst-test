/*
 * test_vbroadcast128.c - Test VBROADCASTF128/VBROADCASTI128 (AVX/AVX2)
 *
 * Both instructions load one 128-bit memory operand and replicate it into
 * the low and high 128-bit lanes of a YMM register.
 *
 * Compile: gcc -o test_vbroadcast128 simd/test_vbroadcast128.c -O0 -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vbroadcastf128(void) {
    static const uint32_t patterns[][4] = {
        { UINT32_C(0x00000000), UINT32_C(0xffffffff),
          UINT32_C(0x80000000), UINT32_C(0x7f800000) },
        { UINT32_C(0x7fc12345), UINT32_C(0x00000001),
          UINT32_C(0x3f800000), UINT32_C(0xbf800000) }
    };

    for (unsigned test = 0; test < sizeof(patterns) / sizeof(patterns[0]); test++) {
        xmm_t src = { .u32 = {
            patterns[test][0], patterns[test][1],
            patterns[test][2], patterns[test][3] } };
        ymm_t dst;

        __asm__ volatile (
            "vbroadcastf128 %1, %%ymm0\n\t"
            "vmovdqu %%ymm0, %0"
            : "=m"(dst) : "m"(src) : "ymm0");

        for (int lane = 0; lane < 2; lane++) {
            for (int i = 0; i < 4; i++) {
                TEST_ASSERT(dst.u32[lane * 4 + i] == patterns[test][i],
                    "vbroadcastf128 test %u lane %d element %d: got %#x expected %#x",
                    test, lane, i, dst.u32[lane * 4 + i], patterns[test][i]);
            }
        }
    }
}

static void test_vbroadcasti128(void) {
    static const uint8_t patterns[][16] = {
        { 0x00, 0x01, 0x02, 0x03, 0x7f, 0x80, 0xfe, 0xff,
          0x10, 0x20, 0x40, 0x80, 0xaa, 0x55, 0xde, 0xad },
        { 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
          0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 }
    };

    for (unsigned test = 0; test < sizeof(patterns) / sizeof(patterns[0]); test++) {
        xmm_t src = { .u8 = {
            patterns[test][0], patterns[test][1], patterns[test][2], patterns[test][3],
            patterns[test][4], patterns[test][5], patterns[test][6], patterns[test][7],
            patterns[test][8], patterns[test][9], patterns[test][10], patterns[test][11],
            patterns[test][12], patterns[test][13], patterns[test][14], patterns[test][15] } };
        ymm_t dst;

        __asm__ volatile (
            "vbroadcasti128 %1, %%ymm0\n\t"
            "vmovdqu %%ymm0, %0"
            : "=m"(dst) : "m"(src) : "ymm0");

        for (int lane = 0; lane < 2; lane++) {
            for (int i = 0; i < 16; i++) {
                TEST_ASSERT(dst.u8[lane * 16 + i] == patterns[test][i],
                    "vbroadcasti128 test %u lane %d byte %d: got %#x expected %#x",
                    test, lane, i, dst.u8[lane * 16 + i], patterns[test][i]);
            }
        }
    }
}

int main(void) {
    TEST_START("VBROADCASTF128/VBROADCASTI128 instructions (AVX/AVX2)");
    test_vbroadcastf128();
    test_vbroadcasti128();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
