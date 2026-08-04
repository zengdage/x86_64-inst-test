/*
 * test_pack.c - Test PACKSSWB/PACKSSDW/PACKUSWB/PACKUSDW instructions
 *
 * PACKSSWB: Pack signed words to signed bytes with saturation.
 * PACKSSDW: Pack signed dwords to signed words with saturation.
 * PACKUSWB: Pack signed words to unsigned bytes with saturation.
 * PACKUSDW: Pack signed dwords to unsigned words with saturation (SSE4.1).
 *
 * Compile: gcc -o test_pack simd/test_pack.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static int8_t sat_i16_i8(int16_t value) {
    if (value > INT8_MAX) return INT8_MAX;
    if (value < INT8_MIN) return INT8_MIN;
    return (int8_t)value;
}

static int16_t sat_i32_i16(int32_t value) {
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static uint8_t sat_i16_u8(int16_t value) {
    if (value < 0) return 0;
    if (value > UINT8_MAX) return UINT8_MAX;
    return (uint8_t)value;
}

static uint16_t sat_i32_u16(int32_t value) {
    if (value < 0) return 0;
    if (value > UINT16_MAX) return UINT16_MAX;
    return (uint16_t)value;
}

static void test_packsswb_no_sat(void) {
    xmm_t a = { .i16 = {1, -1, 50, -50, 100, -100, 127, -128} };
    xmm_t b = { .i16 = {10, -10, 60, -60, 110, -110, 127, -128} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "packsswb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    /* Low 8 bytes from a, high 8 bytes from b */
    TEST_ASSERT(dst.i8[0] == 1, "packsswb a[0]: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -1, "packsswb a[1]: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[6] == 127, "packsswb a[6]: got %d", dst.i8[6]);
    TEST_ASSERT(dst.i8[7] == -128, "packsswb a[7]: got %d", dst.i8[7]);
    TEST_ASSERT(dst.i8[8] == 10, "packsswb b[0]: got %d", dst.i8[8]);
}

static void test_packsswb_saturation(void) {
    xmm_t a = { .i16 = {200, -200, 32767, -32768, 0,0,0,0} };
    xmm_t b = { .i16 = {0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "packsswb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i8[0] == 127, "packsswb 200 saturates to 127: got %d", dst.i8[0]);
    TEST_ASSERT(dst.i8[1] == -128, "packsswb -200 saturates to -128: got %d", dst.i8[1]);
    TEST_ASSERT(dst.i8[2] == 127, "packsswb 32767 saturates to 127: got %d", dst.i8[2]);
    TEST_ASSERT(dst.i8[3] == -128, "packsswb -32768 saturates to -128: got %d", dst.i8[3]);
}

static void test_packssdw_no_sat(void) {
    xmm_t a = { .i32 = {100, -100, 32767, -32768} };
    xmm_t b = { .i32 = {200, -200, 1000, -1000} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "packssdw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 100, "packssdw a[0]: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -100, "packssdw a[1]: got %d", dst.i16[1]);
    TEST_ASSERT(dst.i16[2] == 32767, "packssdw a[2]: got %d", dst.i16[2]);
    TEST_ASSERT(dst.i16[3] == -32768, "packssdw a[3]: got %d", dst.i16[3]);
    TEST_ASSERT(dst.i16[4] == 200, "packssdw b[0]: got %d", dst.i16[4]);
}

static void test_packssdw_saturation(void) {
    xmm_t a = { .i32 = {100000, -100000, 0, 0} };
    xmm_t b = { .i32 = {0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "packssdw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.i16[0] == 32767, "packssdw 100000 saturates to 32767: got %d", dst.i16[0]);
    TEST_ASSERT(dst.i16[1] == -32768, "packssdw -100000 saturates to -32768: got %d", dst.i16[1]);
}

static void test_packuswb_no_sat(void) {
    xmm_t a = { .i16 = {0, 1, 128, 255, 100, 200, 50, 150} };
    xmm_t b = { .i16 = {10, 20, 30, 40, 50, 60, 70, 80} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "packuswb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0, "packuswb a[0]: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[3] == 255, "packuswb a[3]: got %u", dst.u8[3]);
    TEST_ASSERT(dst.u8[8] == 10, "packuswb b[0]: got %u", dst.u8[8]);
}

static void test_packuswb_saturation(void) {
    xmm_t a = { .i16 = {-1, -100, 256, 1000, 0,0,0,0} };
    xmm_t b = { .i16 = {0,0,0,0, 0,0,0,0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "packuswb %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u8[0] == 0, "packuswb -1 saturates to 0: got %u", dst.u8[0]);
    TEST_ASSERT(dst.u8[1] == 0, "packuswb -100 saturates to 0: got %u", dst.u8[1]);
    TEST_ASSERT(dst.u8[2] == 255, "packuswb 256 saturates to 255: got %u", dst.u8[2]);
    TEST_ASSERT(dst.u8[3] == 255, "packuswb 1000 saturates to 255: got %u", dst.u8[3]);
}

static void test_packusdw_basic(void) {
    xmm_t a = { .i32 = {0, 65535, -1, 100000} };
    xmm_t b = { .i32 = {1000, 50000, 0, 0} };
    xmm_t dst;

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "packusdw %2, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u16[0] == 0, "packusdw 0: got %u", dst.u16[0]);
    TEST_ASSERT(dst.u16[1] == 65535, "packusdw 65535: got %u", dst.u16[1]);
    TEST_ASSERT(dst.u16[2] == 0, "packusdw -1 saturates to 0: got %u", dst.u16[2]);
    TEST_ASSERT(dst.u16[3] == 65535, "packusdw 100000 saturates to 65535: got %u", dst.u16[3]);
    TEST_ASSERT(dst.u16[4] == 1000, "packusdw b[0]: got %u", dst.u16[4]);
    TEST_ASSERT(dst.u16[5] == 50000, "packusdw b[1]: got %u", dst.u16[5]);
}

static void test_pack_all_lanes_scalar_oracles_and_register_source(void) {
    xmm_t a = { .i16 = {-129,-128,-127,-1,0,1,127,128} };
    xmm_t b = { .i16 = {INT16_MIN,INT16_MAX,126,129,-130,255,-255,42} };
    xmm_t mem_result, reg_result;

#define RUN_PACK_WORDS(INSN)                                                \
    __asm__ volatile (                                                      \
        "movdqa %2,%%xmm0\n\t" INSN " %3,%%xmm0\n\tmovdqa %%xmm0,%0\n\t" \
        "movdqa %2,%%xmm0\n\tmovdqa %3,%%xmm1\n\t" INSN " %%xmm1,%%xmm0\n\t" \
        "movdqa %%xmm0,%1"                                                \
        : "=m"(mem_result), "=m"(reg_result) : "m"(a), "m"(b)          \
        : "xmm0", "xmm1")

    RUN_PACK_WORDS("packsswb");
    for (int lane = 0; lane < 16; lane++) {
        int16_t source = lane < 8 ? a.i16[lane] : b.i16[lane - 8];
        int8_t expected = sat_i16_i8(source);
        TEST_ASSERT(mem_result.i8[lane] == expected,
                    "packsswb memory scalar oracle lane %d", lane);
        TEST_ASSERT(reg_result.i8[lane] == expected,
                    "packsswb register scalar oracle lane %d", lane);
    }

    RUN_PACK_WORDS("packuswb");
    for (int lane = 0; lane < 16; lane++) {
        int16_t source = lane < 8 ? a.i16[lane] : b.i16[lane - 8];
        uint8_t expected = sat_i16_u8(source);
        TEST_ASSERT(mem_result.u8[lane] == expected,
                    "packuswb memory scalar oracle lane %d", lane);
        TEST_ASSERT(reg_result.u8[lane] == expected,
                    "packuswb register scalar oracle lane %d", lane);
    }
#undef RUN_PACK_WORDS

    a.i32[0] = -32769; a.i32[1] = -32768; a.i32[2] = -32767; a.i32[3] = -1;
    b.i32[0] = 0; b.i32[1] = 32767; b.i32[2] = 32768; b.i32[3] = INT32_MAX;
#define RUN_PACK_DWORDS(INSN)                                               \
    __asm__ volatile (                                                      \
        "movdqa %2,%%xmm0\n\t" INSN " %3,%%xmm0\n\tmovdqa %%xmm0,%0\n\t" \
        "movdqa %2,%%xmm0\n\tmovdqa %3,%%xmm1\n\t" INSN " %%xmm1,%%xmm0\n\t" \
        "movdqa %%xmm0,%1"                                                \
        : "=m"(mem_result), "=m"(reg_result) : "m"(a), "m"(b)          \
        : "xmm0", "xmm1")

    RUN_PACK_DWORDS("packssdw");
    for (int lane = 0; lane < 8; lane++) {
        int32_t source = lane < 4 ? a.i32[lane] : b.i32[lane - 4];
        int16_t expected = sat_i32_i16(source);
        TEST_ASSERT(mem_result.i16[lane] == expected,
                    "packssdw memory scalar oracle lane %d", lane);
        TEST_ASSERT(reg_result.i16[lane] == expected,
                    "packssdw register scalar oracle lane %d", lane);
    }

    RUN_PACK_DWORDS("packusdw");
    for (int lane = 0; lane < 8; lane++) {
        int32_t source = lane < 4 ? a.i32[lane] : b.i32[lane - 4];
        uint16_t expected = sat_i32_u16(source);
        TEST_ASSERT(mem_result.u16[lane] == expected,
                    "packusdw memory scalar oracle lane %d", lane);
        TEST_ASSERT(reg_result.u16[lane] == expected,
                    "packusdw register scalar oracle lane %d", lane);
    }
#undef RUN_PACK_DWORDS
}

int main(void) {
    TEST_START("PACKSSWB/PACKSSDW/PACKUSWB/PACKUSDW instructions");
    test_packsswb_no_sat();
    test_packsswb_saturation();
    test_packssdw_no_sat();
    test_packssdw_saturation();
    test_packuswb_no_sat();
    test_packuswb_saturation();
    test_packusdw_basic();
    test_pack_all_lanes_scalar_oracles_and_register_source();
    TEST_END();
}
