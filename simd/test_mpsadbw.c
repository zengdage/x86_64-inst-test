/*
 * test_mpsadbw.c - Test MPSADBW instruction (SSE4.1)
 *
 * MPSADBW: Multiple packed sums of absolute differences of unsigned bytes.
 * Computes 8 SAD values between a 4-byte block from src2 and 8 consecutive
 * 4-byte blocks from src1. imm8 selects which blocks to use.
 *
 * Compile: gcc -o test_mpsadbw simd/test_mpsadbw.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_mpsadbw_basic(void) {
    xmm_t a = { .u8 = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} };
    xmm_t b = { .u8 = {0,1,2,3, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    /* imm8=0: src1 offset=0, src2 block=0 (bytes 0-3) */
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "mpsadbw $0, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* SAD between b[0:3]={0,1,2,3} and a[0:3]={0,1,2,3} = 0 */
    TEST_ASSERT(dst.u16[0] == 0, "mpsadbw [0] SAD(a[0:3],b[0:3])=0: got %u", dst.u16[0]);
    /* SAD between b[0:3]={0,1,2,3} and a[1:4]={1,2,3,4} = 4 */
    TEST_ASSERT(dst.u16[1] == 4, "mpsadbw [1] SAD(a[1:4],b[0:3])=4: got %u", dst.u16[1]);
    /* SAD between b[0:3] and a[2:5]={2,3,4,5} = 8 */
    TEST_ASSERT(dst.u16[2] == 8, "mpsadbw [2] SAD(a[2:5],b[0:3])=8: got %u", dst.u16[2]);
}

static void test_mpsadbw_identical(void) {
    xmm_t a = { .u8 = {42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42} };
    xmm_t b = { .u8 = {42,42,42,42, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "mpsadbw $0, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u16[i] == 0, "mpsadbw identical [%d]: expected 0, got %u", i, dst.u16[i]);
    }
}

static void test_mpsadbw_max_diff(void) {
    xmm_t a = { .u8 = {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255} };
    xmm_t b = { .u8 = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "mpsadbw $0, %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Each SAD = 4*255 = 1020 */
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.u16[i] == 1020, "mpsadbw max diff [%d]: expected 1020, got %u", i, dst.u16[i]);
    }
}

#define TEST_MPSADBW_CONTROL(imm)                                            \
    do {                                                                     \
        xmm_t dst;                                                           \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "mpsadbw $" #imm ", %2, %%xmm0\n\t"                            \
            "movdqa %%xmm0, %0"                                             \
            : "=m"(dst) : "m"(a), "m"(b) : "xmm0"                        \
        );                                                                   \
        int a_base = (((imm) >> 2) & 1) * 4;                                \
        int b_base = ((imm) & 3) * 4;                                       \
        int ok = 1;                                                         \
        for (int lane = 0; lane < 8; lane++) {                              \
            unsigned expected = 0;                                          \
            for (int j = 0; j < 4; j++) {                                   \
                int diff = (int)a.u8[a_base + lane + j] -                   \
                           (int)b.u8[b_base + j];                            \
                expected += (unsigned)(diff < 0 ? -diff : diff);            \
            }                                                                \
            if (dst.u16[lane] != expected)                                  \
                ok = 0;                                                      \
        }                                                                    \
        TEST_ASSERT(ok, "mpsadbw control $" #imm ": scalar reference mismatch"); \
    } while (0)

static void test_mpsadbw_all_controls(void) {
    xmm_t a = { .u8 = {
        0, 3, 9, 27, 81, 7, 19, 41, 83, 11, 23, 47, 95, 13, 29, 59
    } };
    xmm_t b = { .u8 = {
        2, 5, 10, 20, 40, 80, 160, 1, 17, 34, 68, 136, 15, 31, 63, 127
    } };
    xmm_t imm7, immff;

    TEST_MPSADBW_CONTROL(0);
    TEST_MPSADBW_CONTROL(1);
    TEST_MPSADBW_CONTROL(2);
    TEST_MPSADBW_CONTROL(3);
    TEST_MPSADBW_CONTROL(4);
    TEST_MPSADBW_CONTROL(5);
    TEST_MPSADBW_CONTROL(6);
    TEST_MPSADBW_CONTROL(7);

    __asm__ volatile (
        "movdqa %2, %%xmm0\n\t"
        "mpsadbw $7, %3, %%xmm0\n\t"
        "movdqa %%xmm0, %0\n\t"
        "movdqa %2, %%xmm0\n\t"
        "mpsadbw $0xff, %3, %%xmm0\n\t"
        "movdqa %%xmm0, %1"
        : "=m"(imm7), "=m"(immff)
        : "m"(a), "m"(b)
        : "xmm0"
    );
    TEST_ASSERT(memcmp(&imm7, &immff, sizeof(imm7)) == 0,
                "mpsadbw imm=0xff: high immediate bits ignored");
}

int main(void) {
    TEST_START("MPSADBW instruction (SSE4.1)");
    test_mpsadbw_basic();
    test_mpsadbw_identical();
    test_mpsadbw_max_diff();
    test_mpsadbw_all_controls();
    TEST_END();
}
