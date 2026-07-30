/*
 * test_movntdqa.c - Test SSE4.1 MOVNTDQA instruction
 *
 * MOVNTDQA: Non-temporal load of 128 bits from memory to xmm. Loads 16 bytes
 *           from a 16-byte aligned memory address without polluting the cache.
 *           Operand must be a memory source (register source is undefined).
 *
 * Compile: gcc -o test_movntdqa simd/test_movntdqa.c -O2 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_movntdqa_basic(void) {
    xmm_t mem __attribute__((aligned(16))) = {
        .u32 = {0x11223344u, 0x55667788u, 0x99AABBCCu, 0xDDEEFF00u}
    };
    xmm_t r;
    __asm__ volatile (
        "movntdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(mem) : "xmm0"
    );
    TEST_ASSERT(r.u32[0] == 0x11223344u, "movntdqa [0]: got 0x%x", r.u32[0]);
    TEST_ASSERT(r.u32[1] == 0x55667788u, "movntdqa [1]: got 0x%x", r.u32[1]);
    TEST_ASSERT(r.u32[2] == 0x99AABBCCu, "movntdqa [2]: got 0x%x", r.u32[2]);
    TEST_ASSERT(r.u32[3] == 0xDDEEFF00u, "movntdqa [3]: got 0x%x", r.u32[3]);
}

static void test_movntdqa_bytes(void) {
    xmm_t mem __attribute__((aligned(16))) = {
        .u8 = {0,1,2,3,4,5,6,7, 8,9,10,11,12,13,14,15}
    };
    xmm_t r;
    __asm__ volatile (
        "movntdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(mem) : "xmm0"
    );
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(r.u8[i] == (uint8_t)i, "movntdqa byte [%d]: expected %d, got %u",
                    i, i, r.u8[i]);
    }
}

static void test_movntdqa_floats(void) {
    xmm_t mem __attribute__((aligned(16))) = {
        .f32 = {1.5f, 2.25f, 3.125f, -4.0625f}
    };
    xmm_t r;
    __asm__ volatile (
        "movntdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(mem) : "xmm0"
    );
    TEST_ASSERT(r.f32[0] == 1.5f,    "movntdqa f32 [0]: got %f", r.f32[0]);
    TEST_ASSERT(r.f32[1] == 2.25f,   "movntdqa f32 [1]: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 3.125f,  "movntdqa f32 [2]: got %f", r.f32[2]);
    TEST_ASSERT(r.f32[3] == -4.0625f, "movntdqa f32 [3]: got %f", r.f32[3]);
}

static void test_movntdqa_doubles(void) {
    xmm_t mem __attribute__((aligned(16))) = {
        .f64 = {3.14159, 2.71828}
    };
    xmm_t r;
    __asm__ volatile (
        "movntdqa %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "=m"(r) : "m"(mem) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 3.14159, "movntdqa f64 [0]: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 2.71828, "movntdqa f64 [1]: got %f", r.f64[1]);
}

int main(void) {
    TEST_START("MOVNTDQA");
    test_movntdqa_basic();
    test_movntdqa_bytes();
    test_movntdqa_floats();
    test_movntdqa_doubles();
    TEST_END();
}
