/*
 * test_psubsb_sw.c - Test PSUBSB/PSUBSW/PSUBUSB/PSUBUSW instructions
 *
 * PSUBSB:  Packed subtract signed bytes with saturation.
 * PSUBSW:  Packed subtract signed words with saturation.
 * PSUBUSB: Packed subtract unsigned bytes with saturation.
 * PSUBUSW: Packed subtract unsigned words with saturation.
 *
 * Compile: gcc -o test_psubsb_sw simd/test_psubsb_sw.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_psubsb_no_sat(void) {
    xmm_t a = { .i8 = {50, -50, 100, -100, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {10, -10, 20, -20, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 40, "psubsb 50-10=40: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -40, "psubsb -50-(-10)=-40: got %d", dst.i8[1]);
}

static void test_psubsb_positive_sat(void) {
    xmm_t a = { .i8 = {127, 100, 0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {-1, -100, 0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 127, "psubsb 127-(-1) saturates to 127: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == 127, "psubsb 100-(-100) saturates to 127: got %d", dst.i8[1]);
}

static void test_psubsb_negative_sat(void) {
    xmm_t a = { .i8 = {-128, -100, 0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {1, 100, 0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == -128, "psubsb -128-1 saturates to -128: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -128, "psubsb -100-100 saturates to -128: got %d", dst.i8[1]);
}

static void test_psubsw_positive_sat(void) {
    xmm_t a = { .i16 = {32767, 30000, 0,0, 0,0,0,0} };
    xmm_t b = { .i16 = {-1, -10000, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767, "psubsw 32767-(-1) saturates: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 32767, "psubsw 30000-(-10000) saturates: got %d", dst.i16[1]);
}

static void test_psubsw_negative_sat(void) {
    xmm_t a = { .i16 = {-32768, -30000, 0,0, 0,0,0,0} };
    xmm_t b = { .i16 = {1, 10000, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == -32768, "psubsw -32768-1 saturates: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "psubsw -30000-10000 saturates: got %d", dst.i16[1]);
}

static void test_psubusb_no_sat(void) {
    xmm_t a = { .u8 = {200, 100, 50, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {100, 50, 10, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubusb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 100, "psubusb 200-100=100: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 50, "psubusb 100-50=50: got %u", dst.u8[1]);
}

static void test_psubusb_saturate(void) {
    xmm_t a = { .u8 = {0, 10, 50, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {1, 100, 200, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubusb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0, "psubusb 0-1 saturates to 0: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 0, "psubusb 10-100 saturates to 0: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 0, "psubusb 50-200 saturates to 0: got %u", dst.u8[2]);
}

static void test_psubusw_saturate(void) {
    xmm_t a = { .u16 = {0, 100, 1000, 0, 0,0,0,0} };
    xmm_t b = { .u16 = {1, 200, 65535, 0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psubusw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "psubusw 0-1 saturates to 0: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 0, "psubusw 100-200 saturates to 0: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0, "psubusw 1000-65535 saturates to 0: got %u", dst.u16[2]);
}

int main(void) {
    TEST_START("PSUBSB/PSUBSW/PSUBUSB/PSUBUSW instructions");
    test_psubsb_no_sat();
    test_psubsb_positive_sat();
    test_psubsb_negative_sat();
    test_psubsw_positive_sat();
    test_psubsw_negative_sat();
    test_psubusb_no_sat();
    test_psubusb_saturate();
    test_psubusw_saturate();
    TEST_END();
}
