/*
 * test_paddsb_sw.c - Test PADDSB/PADDSW/PADDUSB/PADDUSW instructions
 *
 * PADDSB:  Packed add signed bytes with saturation.
 * PADDSW:  Packed add signed words with saturation.
 * PADDUSB: Packed add unsigned bytes with saturation.
 * PADDUSW: Packed add unsigned words with saturation.
 *
 * Compile: gcc -o test_paddsb_sw simd/test_paddsb_sw.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_paddsb_no_sat(void) {
    xmm_t a = { .i8 = {10, -10, 50, -50, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {5, -5, 20, -20, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 15, "paddsb 10+5=15: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -15, "paddsb -10+-5=-15: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[2] == 70, "paddsb 50+20=70: got %d", dst.i8[2]);
    TEST_ASSERT(dst.i8[3] == -70, "paddsb -50+-20=-70: got %d", dst.i8[3]);
}

static void test_paddsb_positive_sat(void) {
    xmm_t a = { .i8 = {127, 100, 120, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {1, 100, 50, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 127, "paddsb 127+1 saturates to 127: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == 127, "paddsb 100+100 saturates to 127: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[2] == 127, "paddsb 120+50 saturates to 127: got %d", dst.i8[2]);
}

static void test_paddsb_negative_sat(void) {
    xmm_t a = { .i8 = {-128, -100, -120, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .i8 = {-1, -100, -50, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddsb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == -128, "paddsb -128+-1 saturates to -128: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -128, "paddsb -100+-100 saturates to -128: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[2] == -128, "paddsb -120+-50 saturates to -128: got %d", dst.i8[2]);
}

static void test_paddsw_positive_sat(void) {
    xmm_t a = { .i16 = {32767, 30000, 0,0, 0,0,0,0} };
    xmm_t b = { .i16 = {1, 10000, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767, "paddsw 32767+1 saturates: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 32767, "paddsw 30000+10000 saturates: got %d", dst.i16[1]);
}

static void test_paddsw_negative_sat(void) {
    xmm_t a = { .i16 = {-32768, -30000, 0,0, 0,0,0,0} };
    xmm_t b = { .i16 = {-1, -10000, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == -32768, "paddsw -32768+-1 saturates: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "paddsw -30000+-10000 saturates: got %d", dst.i16[1]);
}

static void test_paddusb_no_sat(void) {
    xmm_t a = { .u8 = {10, 100, 200, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {5, 50, 20, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddusb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 15, "paddusb 10+5=15: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 150, "paddusb 100+50=150: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 220, "paddusb 200+20=220: got %u", dst.u8[2]);
}

static void test_paddusb_saturate(void) {
    xmm_t a = { .u8 = {255, 200, 128, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t b = { .u8 = {1, 100, 200, 0, 0,0,0,0, 0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddusb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 255, "paddusb 255+1 saturates to 255: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 255, "paddusb 200+100 saturates to 255: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 255, "paddusb 128+200 saturates to 255: got %u", dst.u8[2]);
}

static void test_paddusw_saturate(void) {
    xmm_t a = { .u16 = {65535, 60000, 0,0, 0,0,0,0} };
    xmm_t b = { .u16 = {1, 10000, 0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "paddusw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 65535, "paddusw 65535+1 saturates: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 65535, "paddusw 60000+10000 saturates: got %u", dst.u16[1]);
}

int main(void) {
    TEST_START("PADDSB/PADDSW/PADDUSB/PADDUSW instructions");
    test_paddsb_no_sat();
    test_paddsb_positive_sat();
    test_paddsb_negative_sat();
    test_paddsw_positive_sat();
    test_paddsw_negative_sat();
    test_paddusb_no_sat();
    test_paddusb_saturate();
    test_paddusw_saturate();
    TEST_END();
}
