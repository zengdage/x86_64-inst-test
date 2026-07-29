/*
 * test_phadd_phsub.c - Test PHADDW/PHADDD/PHADDSW/PHSUBW/PHSUBD/PHSUBSW (SSSE3)
 *
 * PHADDW:  Packed horizontal add words.
 * PHADDD:  Packed horizontal add dwords.
 * PHADDSW: Packed horizontal add words with saturation.
 * PHSUBW:  Packed horizontal subtract words.
 * PHSUBD:  Packed horizontal subtract dwords.
 * PHSUBSW: Packed horizontal subtract words with saturation.
 *
 * Horizontal operations add/subtract adjacent pairs within operands.
 *
 * Compile: gcc -o test_phadd_phsub simd/test_phadd_phsub.c -O0 -mssse3
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_phaddw(void) {
    xmm_t a = { .i16 = {1, 2, 3, 4, 5, 6, 7, 8} };
    xmm_t b = { .i16 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Low 4: a[0]+a[1], a[2]+a[3], a[4]+a[5], a[6]+a[7] */
    TEST_ASSERT(dst.i16[0] == 3, "phaddw a[0]+a[1]: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 7, "phaddw a[2]+a[3]: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 11, "phaddw a[4]+a[5]: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 15, "phaddw a[6]+a[7]: got %d", dst.i16[3]);
    /* High 4: b[0]+b[1], b[2]+b[3], b[4]+b[5], b[6]+b[7] */
    TEST_ASSERT(dst.i16[4] == 30, "phaddw b[0]+b[1]: got %d", dst.i16[4]);
    TEST_ASSERT(dst.i16[5] == 70, "phaddw b[2]+b[3]: got %d", dst.i16[5]);
    TEST_ASSERT(dst.i16[6] == 110, "phaddw b[4]+b[5]: got %d", dst.i16[6]);
    TEST_ASSERT(dst.i16[7] == 150, "phaddw b[6]+b[7]: got %d", dst.i16[7]);
}

static void test_phaddd(void) {
    xmm_t a = { .i32 = {100, 200, 300, 400} };
    xmm_t b = { .i32 = {1000, 2000, 3000, 4000} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 300, "phaddd a[0]+a[1]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 700, "phaddd a[2]+a[3]: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 3000, "phaddd b[0]+b[1]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 7000, "phaddd b[2]+b[3]: got %d", dst.i32[3]);
}

static void test_phaddsw_saturation(void) {
    xmm_t a = { .i16 = {32767, 1, -32768, -1, 100, 200, -100, -200} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phaddsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767, "phaddsw 32767+1 saturates: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "phaddsw -32768+-1 saturates: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 300, "phaddsw 100+200: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -300, "phaddsw -100+-200: got %d", dst.i16[3]);
}

static void test_phsubw(void) {
    xmm_t a = { .i16 = {10, 3, 20, 5, 30, 7, 40, 9} };
    xmm_t b = { .i16 = {100, 50, 200, 100, 300, 150, 400, 200} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* a[0]-a[1], a[2]-a[3], a[4]-a[5], a[6]-a[7] */
    TEST_ASSERT(dst.i16[0] == 7, "phsubw a[0]-a[1]: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == 15, "phsubw a[2]-a[3]: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 23, "phsubw a[4]-a[5]: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == 31, "phsubw a[6]-a[7]: got %d", dst.i16[3]);
    TEST_ASSERT(dst.i16[4] == 50, "phsubw b[0]-b[1]: got %d", dst.i16[4]);
}

static void test_phsubd(void) {
    xmm_t a = { .i32 = {1000, 300, 5000, 2000} };
    xmm_t b = { .i32 = {100, 100, 500, 200} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubd %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i32[0] == 700, "phsubd a[0]-a[1]: got %d", dst.i32[0]);
    TEST_ASSERT(dst.i32[1] == 3000, "phsubd a[2]-a[3]: got %d", dst.i32[1]);
    TEST_ASSERT(dst.i32[2] == 0, "phsubd b[0]-b[1]: got %d", dst.i32[2]);
    TEST_ASSERT(dst.i32[3] == 300, "phsubd b[2]-b[3]: got %d", dst.i32[3]);
}

static void test_phsubsw_saturation(void) {
    xmm_t a = { .i16 = {32767, -1, -32768, 1, 0,0,0,0} };
    xmm_t b = { .i16 = {0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "phsubsw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 32767 - (-1) = 32768, saturates to 32767 */
    TEST_ASSERT(dst.i16[0] == 32767, "phsubsw 32767-(-1) saturates: got %d", dst.i16[0]);
    /* -32768 - 1 = -32769, saturates to -32768 */
    TEST_ASSERT(dst.i16[1] == -32768, "phsubsw -32768-1 saturates: got %d", dst.i16[1]);
}

int main(void) {
    TEST_START("PHADDW/PHADDD/PHADDSW/PHSUBW/PHSUBD/PHSUBSW instructions (SSSE3)");
    test_phaddw();
    test_phaddd();
    test_phaddsw_saturation();
    test_phsubw();
    test_phsubd();
    test_phsubsw_saturation();
    TEST_END();
}
