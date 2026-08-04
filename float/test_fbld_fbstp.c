/*
 * test_fbld_fbstp.c - Test x87 FBLD/FBSTP instructions (BCD load/store)
 *
 * FBLD: Load 80-bit packed BCD from memory onto FPU stack.
 * FBSTP: Store ST(0) as 80-bit packed BCD to memory and pop.
 * The 80-bit BCD format stores 18 BCD digits (9 bytes) + 1 sign byte.
 *
 * Compile: gcc -o test_fbld_fbstp float/test_fbld_fbstp.c -O0 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>

/* Helper: build a packed BCD value in a 10-byte buffer.
 * val must be non-negative and fit in 18 decimal digits.
 * sign: 0 = positive, 0x80 = negative (stored in byte 9).
 */
static void make_bcd(uint8_t bcd[10], uint64_t val, int negative) {
    memset(bcd, 0, 10);
    for (int i = 0; i < 9 && val > 0; i++) {
        uint8_t lo = val % 10;
        val /= 10;
        uint8_t hi = val % 10;
        val /= 10;
        bcd[i] = (hi << 4) | lo;
    }
    if (negative)
        bcd[9] = 0x80;
}

/* Helper: extract integer from packed BCD */
static int64_t bcd_to_int(const uint8_t bcd[10]) {
    int64_t val = 0;
    int64_t mult = 1;
    for (int i = 0; i < 9; i++) {
        val += (bcd[i] & 0x0F) * mult;
        mult *= 10;
        val += ((bcd[i] >> 4) & 0x0F) * mult;
        mult *= 10;
    }
    if (bcd[9] & 0x80)
        val = -val;
    return val;
}

static void test_fbld(void) {
    uint8_t bcd[10] __attribute__((aligned(16)));
    double result;

    /* Load 0 */
    make_bcd(bcd, 0, 0);
    __asm__ volatile (
        "fbld %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(*bcd)
    );
    TEST_ASSERT(result == 0.0, "fbld 0: got %f", result);

    /* Load 42 */
    make_bcd(bcd, 42, 0);
    __asm__ volatile (
        "fbld %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(*bcd)
    );
    TEST_ASSERT(result == 42.0, "fbld 42: got %f", result);

    /* Load -42 */
    make_bcd(bcd, 42, 1);
    __asm__ volatile (
        "fbld %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(*bcd)
    );
    TEST_ASSERT(result == -42.0, "fbld -42: got %f", result);

    /* Load 999999999 */
    make_bcd(bcd, 999999999ULL, 0);
    __asm__ volatile (
        "fbld %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(*bcd)
    );
    TEST_ASSERT(result == 999999999.0, "fbld 999999999: got %f", result);

    /* Load 123456789012345678 */
    make_bcd(bcd, 123456789012345678ULL, 0);
    __asm__ volatile (
        "fbld %1\n\t"
        "fstpl %0"
        : "=m"(result)
        : "m"(*bcd)
    );
    TEST_ASSERT(result == 123456789012345678.0, "fbld 123456789012345678: got %f", result);
}

static void test_fbstp(void) {
    uint8_t bcd[10] __attribute__((aligned(16)));
    double val;

    /* Store 0 */
    val = 0.0;
    memset(bcd, 0xFF, 10);
    __asm__ volatile (
        "fldl %1\n\t"
        "fbstp %0"
        : "=m"(*bcd)
        : "m"(val)
    );
    TEST_ASSERT(bcd_to_int(bcd) == 0, "fbstp 0: got %ld", bcd_to_int(bcd));

    /* Store 42 */
    val = 42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fbstp %0"
        : "=m"(*bcd)
        : "m"(val)
    );
    TEST_ASSERT(bcd_to_int(bcd) == 42, "fbstp 42: got %ld", bcd_to_int(bcd));

    /* Store -42 */
    val = -42.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fbstp %0"
        : "=m"(*bcd)
        : "m"(val)
    );
    TEST_ASSERT(bcd_to_int(bcd) == -42, "fbstp -42: got %ld", bcd_to_int(bcd));

    /* Store with rounding: 7.5 rounds to 8 (round-to-nearest-even) */
    val = 7.5;
    __asm__ volatile (
        "fldl %1\n\t"
        "fbstp %0"
        : "=m"(*bcd)
        : "m"(val)
    );
    TEST_ASSERT(bcd_to_int(bcd) == 8, "fbstp 7.5 -> 8 (rounding): got %ld", bcd_to_int(bcd));

    /* Store 12345 */
    val = 12345.0;
    __asm__ volatile (
        "fldl %1\n\t"
        "fbstp %0"
        : "=m"(*bcd)
        : "m"(val)
    );
    TEST_ASSERT(bcd_to_int(bcd) == 12345, "fbstp 12345: got %ld", bcd_to_int(bcd));
}

static void test_fbld_fbstp_roundtrip(void) {
    uint8_t bcd_in[10] __attribute__((aligned(16)));
    uint8_t bcd_out[10] __attribute__((aligned(16)));

    /* Round-trip: 9876543210 */
    make_bcd(bcd_in, 9876543210ULL, 0);
    __asm__ volatile (
        "fbld %1\n\t"
        "fbstp %0"
        : "=m"(*bcd_out)
        : "m"(*bcd_in)
    );
    TEST_ASSERT(bcd_to_int(bcd_out) == 9876543210LL,
                "fbld/fbstp roundtrip 9876543210: got %ld", bcd_to_int(bcd_out));

    /* Round-trip: -1234 */
    make_bcd(bcd_in, 1234, 1);
    __asm__ volatile (
        "fbld %1\n\t"
        "fbstp %0"
        : "=m"(*bcd_out)
        : "m"(*bcd_in)
    );
    TEST_ASSERT(bcd_to_int(bcd_out) == -1234,
                "fbld/fbstp roundtrip -1234: got %ld", bcd_to_int(bcd_out));
}

static void test_bcd_18_digit_boundaries(void) {
    uint8_t bcd[10] __attribute__((aligned(16)));
    long double value;
    const uint64_t max_bcd = UINT64_C(999999999999999999);

    make_bcd(bcd, max_bcd, 0);
    __asm__ volatile (
        "fbld %1\n\t"
        "fstpt %0"
        : "=m"(value)
        : "m"(*bcd)
    );
    TEST_ASSERT(value == (long double)max_bcd,
                "fbld largest 18-digit positive BCD exact");

    make_bcd(bcd, max_bcd, 1);
    __asm__ volatile (
        "fbld %1\n\t"
        "fstpt %0"
        : "=m"(value)
        : "m"(*bcd)
    );
    TEST_ASSERT(value == -(long double)max_bcd,
                "fbld largest 18-digit negative BCD exact");

    value = (long double)max_bcd;
    __asm__ volatile (
        "fldt %1\n\t"
        "fbstp %0"
        : "=m"(*bcd)
        : "m"(value)
    );
    TEST_ASSERT(bcd_to_int(bcd) == (int64_t)max_bcd,
                "fbstp largest 18-digit positive BCD");
}

static void test_bcd_signed_zero_rounding_and_invalid(void) {
    uint8_t bcd[10] __attribute__((aligned(16)));
    long double extended;
    uint16_t saved_cw, cw, status;

    make_bcd(bcd, 0, 1);
    __asm__ volatile("fbld %1\n\tfstpt %0" : "=m"(extended) : "m"(*bcd));
    TEST_ASSERT(extended == 0.0L && signbit(extended), "fbld negative packed-BCD zero -> -0");

    extended = -0.0L;
    memset(bcd, 0, sizeof(bcd));
    __asm__ volatile("fldt %1\n\tfbstp %0" : "=m"(*bcd) : "m"(extended));
    TEST_ASSERT(bcd_to_int(bcd) == 0 && (bcd[9] & 0x80),
                "fbstp -0 preserves packed-BCD sign bit");

    __asm__ volatile("fnstcw %0" : "=m"(saved_cw));
    long double positive = 2.5L, negative = -2.5L;
    const int64_t expected[4][2] = {{2,-2}, {2,-3}, {3,-2}, {2,-2}};
    for (uint16_t mode = 0; mode < 4; mode++) {
        cw = (uint16_t)((saved_cw & ~UINT16_C(0x0c00)) | (mode << 10));
        __asm__ volatile("fldcw %0" : : "m"(cw));
        __asm__ volatile("fldt %1\n\tfbstp %0" : "=m"(*bcd) : "m"(positive));
        int64_t pos_result = bcd_to_int(bcd);
        __asm__ volatile("fldt %1\n\tfbstp %0" : "=m"(*bcd) : "m"(negative));
        int64_t neg_result = bcd_to_int(bcd);
        TEST_ASSERT(pos_result == expected[mode][0] && neg_result == expected[mode][1],
                    "fbstp control-word rounding mode %u: %ld %ld",
                    mode, pos_result, neg_result);
    }

    long double out_of_range = 1000000000000000000.0L;
    __asm__ volatile(
        "fnclex\n\tfldt %2\n\tfbstp %0\n\tfnstsw %1"
        : "=m"(*bcd), "=m"(status) : "m"(out_of_range) : "memory"
    );
    TEST_ASSERT(status & 1, "fbstp 19-digit value sets invalid status");

    __asm__ volatile("fldcw %0\n\tfnclex" : : "m"(saved_cw));
}

int main(void) {
    TEST_START("FBLD/FBSTP instructions");
    test_fbld();
    test_fbstp();
    test_fbld_fbstp_roundtrip();
    test_bcd_18_digit_boundaries();
    test_bcd_signed_zero_rounding_and_invalid();
    TEST_END();
}
