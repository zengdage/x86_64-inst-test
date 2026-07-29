/*
 * test_movntdq.c - Test MOVNTDQ/MOVNTI/MOVNTPD/MOVNTPS instructions
 *
 * Non-temporal stores bypass the cache hierarchy (write-combining).
 * MOVNTDQ: Non-temporal store of 128-bit integer data from xmm to memory.
 * MOVNTI:  Non-temporal store of 32/64-bit integer from GPR to memory.
 * MOVNTPD: Non-temporal store of 128-bit double-precision from xmm to memory.
 * MOVNTPS: Non-temporal store of 128-bit single-precision from xmm to memory.
 * All require aligned destination memory.
 *
 * Compile: gcc -o test_movntdq simd/test_movntdq.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movntdq_basic(void) {
    xmm_t src = { .u64 = { 0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL } };
    xmm_t dst __attribute__((aligned(16)));
    memset(&dst, 0, sizeof(dst));

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movntdq %%xmm0, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == src.u64[0] && dst.u64[1] == src.u64[1],
        "movntdq: data mismatch");
}

static void test_movntdq_all_ones(void) {
    xmm_t src = { .u64 = { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL } };
    xmm_t dst __attribute__((aligned(16)));
    memset(&dst, 0, sizeof(dst));

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movntdq %%xmm0, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL && dst.u64[1] == 0xFFFFFFFFFFFFFFFFULL,
        "movntdq all ones: data mismatch");
}

static void test_movntdq_all_zeros(void) {
    xmm_t src = { .u64 = { 0, 0 } };
    xmm_t dst __attribute__((aligned(16)));
    memset(&dst, 0xFF, sizeof(dst));

    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "movntdq %%xmm0, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0 && dst.u64[1] == 0,
        "movntdq all zeros: data mismatch");
}

static void test_movnti_32(void) {
    uint32_t dst __attribute__((aligned(4))) = 0;
    uint32_t val = 0xDEADBEEF;

    __asm__ volatile (
        "movnti %1, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "r"(val)
    );
    TEST_ASSERT(dst == val, "movnti 32-bit: expected 0x%08x, got 0x%08x", val, dst);
}

static void test_movnti_64(void) {
    uint64_t dst __attribute__((aligned(8))) = 0;
    uint64_t val = 0xCAFEBABEDEADBEEFULL;

    __asm__ volatile (
        "movnti %1, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "r"(val)
    );
    TEST_ASSERT(dst == val, "movnti 64-bit: expected 0x%016lx, got 0x%016lx", val, dst);
}

static void test_movntpd(void) {
    xmm_t src = { .f64 = { 3.14159, 2.71828 } };
    xmm_t dst __attribute__((aligned(16)));
    memset(&dst, 0, sizeof(dst));

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movntpd %%xmm0, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    TEST_ASSERT(dst.f64[0] == src.f64[0] && dst.f64[1] == src.f64[1],
        "movntpd: data mismatch");
}

static void test_movntps(void) {
    xmm_t src = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t dst __attribute__((aligned(16)));
    memset(&dst, 0, sizeof(dst));

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movntps %%xmm0, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "m"(src)
        : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(dst.f32[i] == src.f32[i],
            "movntps [%d]: expected %f, got %f", i, src.f32[i], dst.f32[i]);
    }
}

static void test_movnti_boundary(void) {
    uint64_t dst __attribute__((aligned(8))) = 0;
    uint64_t val = 0;

    __asm__ volatile (
        "movnti %1, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "r"(val)
    );
    TEST_ASSERT(dst == 0, "movnti zero: expected 0, got 0x%016lx", dst);

    val = 0xFFFFFFFFFFFFFFFFULL;
    __asm__ volatile (
        "movnti %1, %0\n\t"
        "sfence"
        : "=m"(dst)
        : "r"(val)
    );
    TEST_ASSERT(dst == 0xFFFFFFFFFFFFFFFFULL, "movnti max: expected all 1s, got 0x%016lx", dst);
}

int main(void) {
    TEST_START("MOVNTDQ/MOVNTI/MOVNTPD/MOVNTPS instructions");
    test_movntdq_basic();
    test_movntdq_all_ones();
    test_movntdq_all_zeros();
    test_movnti_32();
    test_movnti_64();
    test_movntpd();
    test_movntps();
    test_movnti_boundary();
    TEST_END();
}
