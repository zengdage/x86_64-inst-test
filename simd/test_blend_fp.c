/*
 * test_blend_fp.c - Test SSE4.1 floating-point blend instructions
 *
 * BLENDPS : Blend packed single-precision floats using imm8 mask (2 bits per dword).
 * BLENDPD : Blend packed double-precision floats using imm8 mask (1 bit per qword).
 * BLENDVPS: Variable blend packed single-precision floats using implicit XMM0 mask.
 * BLENDVPD: Variable blend packed double-precision floats using implicit XMM0 mask.
 *
 * Compile: gcc -o test_blend_fp simd/test_blend_fp.c -O2 -msse4.1 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>

static void test_blendps_none(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} };
    xmm_t r;
    /* imm8=0: all from dest (a) */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "blendps $0x0, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(r.f32[i] == a.f32[i], "blendps $0 [%d]: got %f", i, r.f32[i]);
    }
}

static void test_blendps_all(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} };
    xmm_t r;
    /* imm8=0xF: all from src (b) */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "blendps $0xF, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(r.f32[i] == b.f32[i], "blendps $F [%d]: got %f", i, r.f32[i]);
    }
}

static void test_blendps_mixed(void) {
    xmm_t a = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} };
    xmm_t r;
    /* imm8=0b0101 = 0x5: dword 0,2 from src; dword 1,3 from dest */
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "blendps $0x5, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(r.f32[0] == 10.0f, "blendps $5 [0] from src: got %f", r.f32[0]);
    TEST_ASSERT(r.f32[1] == 2.0f,  "blendps $5 [1] from dest: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 30.0f, "blendps $5 [2] from src: got %f", r.f32[2]);
    TEST_ASSERT(r.f32[3] == 4.0f,  "blendps $5 [3] from dest: got %f", r.f32[3]);
}

static void test_blendpd_none(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {10.0, 20.0} };
    xmm_t r;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "blendpd $0x0, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 1.0, "blendpd $0 [0] from dest: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 2.0, "blendpd $0 [1] from dest: got %f", r.f64[1]);
}

static void test_blendpd_all(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {10.0, 20.0} };
    xmm_t r;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "blendpd $0x3, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 10.0, "blendpd $3 [0] from src: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 20.0, "blendpd $3 [1] from src: got %f", r.f64[1]);
}

static void test_blendpd_mixed(void) {
    xmm_t a = { .f64 = {1.0, 2.0} };
    xmm_t b = { .f64 = {10.0, 20.0} };
    xmm_t r;
    /* imm8=0b10 = 0x2: qword 0 from dest, qword 1 from src */
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "blendpd $0x2, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 1.0,  "blendpd $2 [0] from dest: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 20.0, "blendpd $2 [1] from src: got %f",  r.f64[1]);
}

static void test_blendvps_all_src(void) {
    xmm_t a   = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b   = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} };
    xmm_t mask = { .u32 = {0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u} };
    xmm_t r;
    /* mask must be in xmm0; use dest=xmm1, src=mem */
    __asm__ volatile (
        "movaps %3, %%xmm0\n\t"
        "movaps %1, %%xmm1\n\t"
        "blendvps %%xmm0, %2, %%xmm1\n\t"
        "movaps %%xmm1, %0"
        : "=m"(r) : "m"(a), "m"(b), "m"(mask) : "xmm0", "xmm1"
    );
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT(r.f32[i] == b.f32[i], "blendvps all src [%d]: got %f", i, r.f32[i]);
    }
}

static void test_blendvps_mixed(void) {
    xmm_t a   = { .f32 = {1.0f, 2.0f, 3.0f, 4.0f} };
    xmm_t b   = { .f32 = {10.0f, 20.0f, 30.0f, 40.0f} };
    xmm_t mask = { .u32 = {0x80000000u, 0u, 0x80000000u, 0u} };
    xmm_t r;
    __asm__ volatile (
        "movaps %3, %%xmm0\n\t"
        "movaps %1, %%xmm1\n\t"
        "blendvps %%xmm0, %2, %%xmm1\n\t"
        "movaps %%xmm1, %0"
        : "=m"(r) : "m"(a), "m"(b), "m"(mask) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.f32[0] == 10.0f, "blendvps mixed [0] from src: got %f", r.f32[0]);
    TEST_ASSERT(r.f32[1] == 2.0f,  "blendvps mixed [1] from dest: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 30.0f, "blendvps mixed [2] from src: got %f", r.f32[2]);
    TEST_ASSERT(r.f32[3] == 4.0f,  "blendvps mixed [3] from dest: got %f", r.f32[3]);
}

static void test_blendvpd_mixed(void) {
    xmm_t a   = { .f64 = {1.0, 2.0} };
    xmm_t b   = { .f64 = {10.0, 20.0} };
    xmm_t mask = { .u64 = {0x8000000000000000ULL, 0ULL} };
    xmm_t r;
    __asm__ volatile (
        "movapd %3, %%xmm0\n\t"
        "movapd %1, %%xmm1\n\t"
        "blendvpd %%xmm0, %2, %%xmm1\n\t"
        "movapd %%xmm1, %0"
        : "=m"(r) : "m"(a), "m"(b), "m"(mask) : "xmm0", "xmm1"
    );
    TEST_ASSERT(r.f64[0] == 10.0, "blendvpd mixed [0] from src: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 2.0,  "blendvpd mixed [1] from dest: got %f", r.f64[1]);
}

int main(void) {
    TEST_START("BLENDPS/BLENDPD/BLENDVPS/BLENDVPD");
    test_blendps_none();
    test_blendps_all();
    test_blendps_mixed();
    test_blendpd_none();
    test_blendpd_all();
    test_blendpd_mixed();
    test_blendvps_all_src();
    test_blendvps_mixed();
    test_blendvpd_mixed();
    TEST_END();
}
