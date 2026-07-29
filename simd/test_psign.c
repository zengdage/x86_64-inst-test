/*
 * test_psign.c - Test PSIGNB/PSIGNW/PSIGND instructions (SSSE3)
 *
 * PSIGNx: For each element, if mask element is negative, negate the data element;
 * if mask element is zero, zero the data element; if positive, keep data unchanged.
 *
 * Compile: gcc -o test_psign simd/test_psign.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_psignb(void) {
    xmm_t a = { .i8 = {10, -10, 50, -50, 0, 127, -128, 1, 0,0,0,0, 0,0,0,0} };
    xmm_t mask = { .i8 = {1, 1, -1, -1, 1, 0, 0, -1, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psignb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(mask) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 10, "psignb pos*pos=pos: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -10, "psignb neg*pos=neg: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[2] == -50, "psignb pos*neg=neg: got %d", dst.i8[2]);
    TEST_ASSERT(dst.i8[3] == 50, "psignb neg*neg=pos: got %d", dst.i8[3]);
    TEST_ASSERT(dst.i8[4] == 0, "psignb 0*pos=0: got %d", dst.i8[4]);
    TEST_ASSERT(dst.i8[5] == 0, "psignb pos*0=0: got %d", dst.i8[5]);
    TEST_ASSERT(dst.i8[6] == 0, "psignb neg*0=0: got %d", dst.i8[6]);
    TEST_ASSERT(dst.i8[7] == -1, "psignb 1*neg=-1: got %d", dst.i8[7]);
}

static void test_psignw(void) {
    xmm_t a = { .i16 = {100, -100, 32767, -32768, 0, 1, -1, 0} };
    xmm_t mask = { .i16 = {1, -1, -1, 1, 1, 0, 0, -1} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psignw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(mask) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 100, "psignw pos*pos: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 100, "psignw neg*neg: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == -32767, "psignw max*neg: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -32768, "psignw min*pos: got %d", dst.i16[3]);
    TEST_ASSERT(dst.i16[5] == 0, "psignw *0: got %d", dst.i16[5]);
    TEST_ASSERT(dst.i16[6] == 0, "psignw *0: got %d", dst.i16[6]);
}

static void test_psignd(void) {
    xmm_t a = { .i32 = {100, -100, 0, 0x7FFFFFFF} };
    xmm_t mask = { .i32 = {1, -1, -1, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psignd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(mask) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 100, "psignd pos*pos: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 100, "psignd neg*neg: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 0, "psignd 0*neg: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 0, "psignd max*0: got %d", dst.i32[3]);
}

static void test_psignb_min_negate(void) {
    /* Negating -128 wraps to -128 */
    xmm_t a = { .i8 = {-128, 0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t mask = { .i8 = {-1, 0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psignb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(mask) : "xmm0"
    );
    /* -(-128) overflows in 8-bit signed, result is -128 */
    TEST_ASSERT(dst.i8[0] == -128, "psignb negate(-128)=-128 (overflow): got %d", dst.i8[0]);
}

int main(void) {
    TEST_START("PSIGNB/PSIGNW/PSIGND instructions (SSSE3)");
    test_psignb();
    test_psignw();
    test_psignd();
    test_psignb_min_negate();
    TEST_END();
}
