/*
 * test_cmpps_pd.c - Test CMPPS/CMPPD/CMPSS/CMPSD instructions
 *
 * These compare packed/scalar float/double values using an immediate predicate:
 *   0=EQ, 1=LT, 2=LE, 3=UNORD, 4=NEQ, 5=NLT, 6=NLE, 7=ORD
 * Result is all 1s (true) or all 0s (false) per element.
 *
 * Compile: gcc -o test_cmpps_pd simd/test_cmpps_pd.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>

static void test_cmpps_eq(void) {
    xmm_t a = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t b = { .f32 = { 1.0f, 0.0f, 3.0f, 5.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "cmpps $0, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "cmpps EQ 1==1: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0x00000000, "cmpps EQ 2!=0: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0xFFFFFFFF, "cmpps EQ 3==3: got 0x%08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0x00000000, "cmpps EQ 4!=5: got 0x%08x", dst.u32[3]);
}

static void test_cmpps_lt(void) {
    xmm_t a = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t b = { .f32 = { 2.0f, 2.0f, 1.0f, 5.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "cmpps $1, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "cmpps LT 1<2: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0x00000000, "cmpps LT 2==2: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0x00000000, "cmpps LT 3>1: got 0x%08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0xFFFFFFFF, "cmpps LT 4<5: got 0x%08x", dst.u32[3]);
}

static void test_cmpps_le(void) {
    xmm_t a = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t b = { .f32 = { 2.0f, 2.0f, 1.0f, 4.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "cmpps $2, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "cmpps LE 1<=2: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0xFFFFFFFF, "cmpps LE 2<=2: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0x00000000, "cmpps LE 3>1: got 0x%08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0xFFFFFFFF, "cmpps LE 4<=4: got 0x%08x", dst.u32[3]);
}

static void test_cmpps_neq(void) {
    xmm_t a = { .f32 = { 1.0f, 2.0f, 3.0f, 4.0f } };
    xmm_t b = { .f32 = { 1.0f, 0.0f, 3.0f, 5.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "cmpps $4, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0x00000000, "cmpps NEQ 1==1: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0xFFFFFFFF, "cmpps NEQ 2!=0: got 0x%08x", dst.u32[1]);
}

static void test_cmppd_eq(void) {
    xmm_t a = { .f64 = { 1.0, 2.0 } };
    xmm_t b = { .f64 = { 1.0, 3.0 } };
    xmm_t dst;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "cmppd $0, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL, "cmppd EQ 1==1: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x0000000000000000ULL, "cmppd EQ 2!=3: got 0x%016lx", dst.u64[1]);
}

static void test_cmppd_lt(void) {
    xmm_t a = { .f64 = { 1.0, 3.0 } };
    xmm_t b = { .f64 = { 2.0, 2.0 } };
    xmm_t dst;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "cmppd $1, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL, "cmppd LT 1<2: got 0x%016lx", dst.u64[0]);
    TEST_ASSERT(dst.u64[1] == 0x0000000000000000ULL, "cmppd LT 3>2: got 0x%016lx", dst.u64[1]);
}

static void test_cmpss_eq(void) {
    xmm_t a = { .f32 = { 1.0f, 99.0f, 99.0f, 99.0f } };
    xmm_t b = { .f32 = { 1.0f, 0.0f, 0.0f, 0.0f } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "cmpss $0, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "cmpss EQ 1==1: got 0x%08x", dst.u32[0]);
    /* Upper elements unchanged */
    TEST_ASSERT(dst.f32[1] == 99.0f, "cmpss upper unchanged [1]: got %f", dst.f32[1]);
}

static void test_cmpsd_lt(void) {
    xmm_t a = { .f64 = { 1.0, 99.0 } };
    xmm_t b = { .f64 = { 2.0, 0.0 } };
    xmm_t dst;

    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "cmpsd $1, %2, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL, "cmpsd LT 1<2: got 0x%016lx", dst.u64[0]);
    /* Upper element unchanged */
    TEST_ASSERT(dst.f64[1] == 99.0, "cmpsd upper unchanged: got %f", dst.f64[1]);
}

static void test_cmpps_unord(void) {
    xmm_t a = { .f32 = { 1.0f, NAN, NAN, 1.0f } };
    xmm_t b = { .f32 = { 1.0f, 1.0f, NAN, NAN } };
    xmm_t dst;

    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "cmpps $3, %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0x00000000, "cmpps UNORD 1,1 ordered: got 0x%08x", dst.u32[0]);
    TEST_ASSERT(dst.u32[1] == 0xFFFFFFFF, "cmpps UNORD NaN,1 unordered: got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0xFFFFFFFF, "cmpps UNORD NaN,NaN unordered: got 0x%08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[3] == 0xFFFFFFFF, "cmpps UNORD 1,NaN unordered: got 0x%08x", dst.u32[3]);
}

int main(void) {
    TEST_START("CMPPS/CMPPD/CMPSS/CMPSD instructions");
    test_cmpps_eq();
    test_cmpps_lt();
    test_cmpps_le();
    test_cmpps_neq();
    test_cmppd_eq();
    test_cmppd_lt();
    test_cmpss_eq();
    test_cmpsd_lt();
    test_cmpps_unord();
    TEST_END();
}
