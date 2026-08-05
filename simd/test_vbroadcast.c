/*
 * test_vbroadcast.c - Test VBROADCASTSS/VBROADCASTSD instructions (AVX)
 *
 * VBROADCASTSS: Broadcast a single float to all elements of ymm (from mem or xmm).
 * VBROADCASTSD: Broadcast a double to all elements of ymm (from mem or xmm).
 *
 * Compile: gcc -o test_vbroadcast simd/test_vbroadcast.c -O0 -mavx2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vbroadcastss_mem(void) {
    float val = 3.14f;
    ymm_t dst;

    __asm__ volatile (
        "vbroadcastss %1, %%ymm0\n\t"
        "vmovaps %%ymm0, %0"
        : "=m"(dst) : "m"(val) : "ymm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == 3.14f,
            "vbroadcastss mem [%d]: expected 3.14, got %f", i, dst.f32[i]);
    }
}

static void test_vbroadcastsd_mem(void) {
    double val = 2.71828;
    ymm_t dst;

    __asm__ volatile (
        "vbroadcastsd %1, %%ymm0\n\t"
        "vmovapd %%ymm0, %0"
        : "=m"(dst) : "m"(val) : "ymm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f64[i] == 2.71828,
            "vbroadcastsd mem [%d]: expected 2.71828, got %f", i, dst.f64[i]);
    }
}

static void test_vbroadcastss_xmm(void) {
    xmm_t src = { .f32 = {42.0f, 0.0f, 0.0f, 0.0f} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%xmm0\n\t"
        "vbroadcastss %%xmm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "xmm0", "ymm1"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == 42.0f,
            "vbroadcastss xmm [%d]: expected 42.0, got %f", i, dst.f32[i]);
    }
}

static void test_vbroadcastss_zero(void) {
    float val = 0.0f;
    ymm_t dst;
    memset(&dst, 0xFF, sizeof(dst));

    __asm__ volatile (
        "vbroadcastss %1, %%ymm0\n\t"
        "vmovaps %%ymm0, %0"
        : "=m"(dst) : "m"(val) : "ymm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == 0.0f,
            "vbroadcastss zero [%d]: got %f", i, dst.f32[i]);
    }
}

static void test_vbroadcastss_negative(void) {
    float val = -99.5f;
    ymm_t dst;

    __asm__ volatile (
        "vbroadcastss %1, %%ymm0\n\t"
        "vmovaps %%ymm0, %0"
        : "=m"(dst) : "m"(val) : "ymm0"
    );
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(dst.f32[i] == -99.5f,
            "vbroadcastss negative [%d]: got %f", i, dst.f32[i]);
    }
}

static void test_vbroadcastsd_xmm(void) {
    xmm_t src = { .f64 = {1.23456789, 0.0} };
    ymm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%xmm0\n\t"
        "vbroadcastsd %%xmm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "xmm0", "ymm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f64[i] == 1.23456789,
            "vbroadcastsd xmm [%d]: got %f", i, dst.f64[i]);
    }
}

static void test_vbroadcast_bit_patterns(void) {
    static const uint32_t fbits[] = {
        UINT32_C(0x80000000), /* -0 */
        UINT32_C(0x7f800000), /* +inf */
        UINT32_C(0xff800000), /* -inf */
        UINT32_C(0x7fc12345), /* qNaN payload */
        UINT32_C(0x00000001)  /* minimum subnormal */
    };
    for (unsigned test = 0; test < sizeof(fbits) / sizeof(fbits[0]); test++) {
        uint32_t value = fbits[test];
        ymm_t dst;
        __asm__ volatile (
            "vbroadcastss %1, %%ymm0\n\t" "vmovdqu %%ymm0, %0"
            : "=m"(dst) : "m"(value) : "ymm0");
        for (int i = 0; i < 8; i++)
            TEST_ASSERT(dst.u32[i] == value,
                        "vbroadcastss bit pattern %#x lane %d got %#x", value, i, dst.u32[i]);
        if (value == UINT32_C(0x7fc12345))
            for (int i = 0; i < 8; i++)
                TEST_ASSERT(IS_QNAN(dst.f32[i]), "vbroadcastss QNaN lane %d", i);
    }

    static const uint64_t dbits[] = {
        UINT64_C(0x8000000000000000),
        UINT64_C(0x7ff0000000000000),
        UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8123456789abc),
        UINT64_C(0x0000000000000001)
    };
    for (unsigned test = 0; test < sizeof(dbits) / sizeof(dbits[0]); test++) {
        uint64_t value = dbits[test];
        ymm_t dst;
        __asm__ volatile (
            "vbroadcastsd %1, %%ymm0\n\t" "vmovdqu %%ymm0, %0"
            : "=m"(dst) : "m"(value) : "ymm0");
        for (int i = 0; i < 4; i++)
            TEST_ASSERT(dst.u64[i] == value,
                        "vbroadcastsd bit pattern %#" PRIx64 " lane %d got %#" PRIx64,
                        value, i, dst.u64[i]);
        if (value == UINT64_C(0x7ff8123456789abc))
            for (int i = 0; i < 4; i++)
                TEST_ASSERT(IS_QNAN(dst.f64[i]), "vbroadcastsd QNaN lane %d", i);
    }

    /* Register source must use only the low element. */
    xmm_t src = { .u32 = {UINT32_C(0x80000000), UINT32_C(0x7fc00001),
                           UINT32_MAX, UINT32_C(0x3f800000)} };
    ymm_t dst;
    __asm__ volatile (
        "vmovdqu %1, %%xmm0\n\t" "vbroadcastss %%xmm0, %%ymm1\n\t" "vmovdqu %%ymm1, %0"
        : "=m"(dst) : "m"(src) : "xmm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst.u32[i] == UINT32_C(0x80000000),
                    "vbroadcastss ignores upper source elements lane %d", i);
}

int main(void) {
    TEST_START("VBROADCASTSS/VBROADCASTSD instructions (AVX)");
    test_vbroadcastss_mem();
    test_vbroadcastsd_mem();
    test_vbroadcastss_xmm();
    test_vbroadcastss_zero();
    test_vbroadcastss_negative();
    test_vbroadcastsd_xmm();
    test_vbroadcast_bit_patterns();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
