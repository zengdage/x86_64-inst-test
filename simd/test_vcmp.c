/*
 * test_vcmp.c - Test VCMPPS/VCMPPD instructions (AVX)
 *
 * AVX comparison with 32 predicates (imm8 0-31).
 * Common predicates: 0=EQ_OQ, 1=LT_OS, 2=LE_OS, 4=NEQ_UQ, etc.
 * Result: all 1s (true) or all 0s (false) per element.
 *
 * Compile: gcc -o test_vcmp simd/test_vcmp.c -O0 -mavx
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_vcmpps_eq_256(void) {
    ymm_t a = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t b = { .f32 = {1,0,3,0,5,0,7,0} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vcmpps $0, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "vcmpps EQ [0] 1==1");
    TEST_ASSERT(dst.u32[1] == 0x00000000, "vcmpps EQ [1] 2!=0");
    TEST_ASSERT(dst.u32[2] == 0xFFFFFFFF, "vcmpps EQ [2] 3==3");
    TEST_ASSERT(dst.u32[3] == 0x00000000, "vcmpps EQ [3] 4!=0");
    TEST_ASSERT(dst.u32[4] == 0xFFFFFFFF, "vcmpps EQ [4] 5==5");
    TEST_ASSERT(dst.u32[5] == 0x00000000, "vcmpps EQ [5] 6!=0");
}

static void test_vcmpps_lt_256(void) {
    ymm_t a = { .f32 = {1,5,3,7,2,6,4,8} };
    ymm_t b = { .f32 = {2,4,4,6,3,5,5,7} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vcmpps $1, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.u32[0] == 0xFFFFFFFF, "vcmpps LT 1<2");
    TEST_ASSERT(dst.u32[1] == 0x00000000, "vcmpps LT 5>4");
    TEST_ASSERT(dst.u32[2] == 0xFFFFFFFF, "vcmpps LT 3<4");
    TEST_ASSERT(dst.u32[3] == 0x00000000, "vcmpps LT 7>6");
}

static void test_vcmppd_eq_256(void) {
    ymm_t a = { .f64 = {1.0, 2.0, 3.0, 4.0} };
    ymm_t b = { .f64 = {1.0, 0.0, 3.0, 0.0} };
    ymm_t dst;

    __asm__ volatile (
        "vmovapd %1, %%ymm0\n\t"
        "vcmppd $0, %2, %%ymm0, %%ymm1\n\t"
        "vmovapd %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.u64[0] == 0xFFFFFFFFFFFFFFFFULL, "vcmppd EQ 1==1");
    TEST_ASSERT(dst.u64[1] == 0, "vcmppd EQ 2!=0");
    TEST_ASSERT(dst.u64[2] == 0xFFFFFFFFFFFFFFFFULL, "vcmppd EQ 3==3");
    TEST_ASSERT(dst.u64[3] == 0, "vcmppd EQ 4!=0");
}

static void test_vcmpps_neq(void) {
    ymm_t a = { .f32 = {1,2,3,4,5,6,7,8} };
    ymm_t b = { .f32 = {1,0,3,0,5,0,7,0} };
    ymm_t dst;

    __asm__ volatile (
        "vmovaps %1, %%ymm0\n\t"
        "vcmpps $4, %2, %%ymm0, %%ymm1\n\t"
        "vmovaps %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    TEST_ASSERT(dst.u32[0] == 0x00000000, "vcmpps NEQ [0] 1==1: not neq");
    TEST_ASSERT(dst.u32[1] == 0xFFFFFFFF, "vcmpps NEQ [1] 2!=0: neq");
}

int main(void) {
    TEST_START("VCMPPS/VCMPPD instructions (AVX)");
    test_vcmpps_eq_256();
    test_vcmpps_lt_256();
    test_vcmppd_eq_256();
    test_vcmpps_neq();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
