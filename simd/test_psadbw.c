/*
 * test_psadbw.c - Test PSADBW instruction
 *
 * PSADBW: Compute sum of absolute differences of unsigned bytes.
 * For each 8-byte group (low and high), computes sum of |a[i]-b[i]| and
 * stores the result as a 16-bit value in the corresponding qword (zero-extended).
 *
 * Compile: gcc -o test_psadbw simd/test_psadbw.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_psadbw_zero(void) {
    xmm_t a = { .u8 = {1,2,3,4,5,6,7,8, 9,10,11,12,13,14,15,16} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psadbw %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0, "psadbw same data low: got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "psadbw same data high: got %lu", dst.u64[1]);
}

static void test_psadbw_basic(void) {
    xmm_t a = { .u8 = {10,20,30,40,50,60,70,80, 0,0,0,0,0,0,0,0} };
    xmm_t b = { .u8 = {11,22,33,44,55,66,77,88, 0,0,0,0,0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psadbw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* |10-11|+|20-22|+|30-33|+|40-44|+|50-55|+|60-66|+|70-77|+|80-88| */
    /* = 1+2+3+4+5+6+7+8 = 36 */
    TEST_ASSERT(dst.u64[0] == 36, "psadbw low: expected 36, got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0, "psadbw high zeros: got %lu", dst.u64[1]);
}

static void test_psadbw_max(void) {
    xmm_t a = { .u8 = {255,255,255,255,255,255,255,255, 0,0,0,0,0,0,0,0} };
    xmm_t b = { .u8 = {0,0,0,0,0,0,0,0, 255,255,255,255,255,255,255,255} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psadbw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* 8 * 255 = 2040 */
    TEST_ASSERT(dst.u64[0] == 2040, "psadbw max low: expected 2040, got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 2040, "psadbw max high: expected 2040, got %lu", dst.u64[1]);
}

static void test_psadbw_both_halves(void) {
    xmm_t a = { .u8 = {10,10,10,10,10,10,10,10, 20,20,20,20,20,20,20,20} };
    xmm_t b = { .u8 = {5,5,5,5,5,5,5,5, 10,10,10,10,10,10,10,10} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "psadbw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 40, "psadbw low: 8*5=40, got %lu", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 80, "psadbw high: 8*10=80, got %lu", dst.u64[1]);
}

static uint64_t psadbw_reference(const xmm_t *a, const xmm_t *b, int group) {
    uint64_t sum = 0;
    for (int i = 0; i < 8; i++) {
        int diff = (int)a->u8[group * 8 + i] - (int)b->u8[group * 8 + i];
        sum += (uint64_t)(diff < 0 ? -diff : diff);
    }
    return sum;
}

static void test_psadbw_scalar_reference_and_register_source(void) {
    xmm_t a = { .u8 = {
        0, 255, 1, 254, 17, 33, 129, 200,
        5, 11, 23, 47, 95, 191, 250, 3
    } };
    xmm_t b = { .u8 = {
        255, 0, 2, 253, 99, 1, 128, 201,
        250, 7, 31, 63, 127, 0, 249, 4
    } };
    xmm_t memory_result, register_result;

    __asm__ volatile (
        "movdqa %2, %%xmm0\n\t"
        "psadbw %3, %%xmm0\n\t"
        "movdqa %%xmm0, %0\n\t"
        "movdqa %2, %%xmm0\n\t"
        "movdqa %3, %%xmm1\n\t"
        "psadbw %%xmm1, %%xmm0\n\t"
        "movdqa %%xmm0, %1"
        : "=m"(memory_result), "=m"(register_result)
        : "m"(a), "m"(b)
        : "xmm0", "xmm1"
    );
    for (int group = 0; group < 2; group++) {
        uint64_t expected = psadbw_reference(&a, &b, group);
        TEST_ASSERT(memory_result.u64[group] == expected,
                    "psadbw scalar reference memory source group %d: %" PRIu64 " != %" PRIu64,
                    group, memory_result.u64[group], expected);
        TEST_ASSERT(register_result.u64[group] == expected,
                    "psadbw scalar reference register source group %d: %" PRIu64 " != %" PRIu64,
                    group, register_result.u64[group], expected);
    }
}

int main(void) {
    TEST_START("PSADBW instruction");
    test_psadbw_zero();
    test_psadbw_basic();
    test_psadbw_max();
    test_psadbw_both_halves();
    test_psadbw_scalar_reference_and_register_source();
    TEST_END();
}
