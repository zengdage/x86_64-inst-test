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
#include <math.h>

#define RUN_VCMPPS_PRED(pred, out, lhs, rhs) do { \
    __asm__ volatile ( \
        "vmovaps %1, %%ymm0\n\t" \
        "vcmpps $" #pred ", %2, %%ymm0, %%ymm1\n\t" \
        "vmovaps %%ymm1, %0" \
        : "=m"(out) : "m"(lhs), "m"(rhs) : "ymm0", "ymm1"); \
} while (0)

static int cmp_predicate_expected(float a, float b, int pred) {
    int unord = isnan(a) || isnan(b);
    switch (pred & 31) {
    case 0: case 16: return !unord && a == b;
    case 1: case 17: return !unord && a < b;
    case 2: case 18: return !unord && a <= b;
    case 3: case 19: return unord;
    case 4: case 20: return unord || a != b;
    case 5: case 21: return unord || !(a < b);
    case 6: case 22: return unord || !(a <= b);
    case 7: case 23: return !unord;
    case 8: case 24: return unord || a == b;
    case 9: case 25: return unord || !(a >= b);
    case 10: case 26: return unord || !(a > b);
    case 11: case 27: return 0;
    case 12: case 28: return !unord && a != b;
    case 13: case 29: return !unord && a >= b;
    case 14: case 30: return !unord && a > b;
    default: return 1;
    }
}

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

static void test_vcmpps_all_predicates(void) {
    ymm_t a = { .f32 = {1.0f, 1.0f, 2.0f, NAN, 0.0f, -0.0f, INFINITY, -INFINITY} };
    ymm_t b = { .f32 = {1.0f, 2.0f, 1.0f, 1.0f, -0.0f, 0.0f, INFINITY, INFINITY} };
    ymm_t dst;
#define CHECK_PRED(n) do { \
    RUN_VCMPPS_PRED(n, dst, a, b); \
    for (int i = 0; i < 8; i++) { \
        uint32_t expected = cmp_predicate_expected(a.f32[i], b.f32[i], n) ? UINT32_MAX : 0; \
        TEST_ASSERT(dst.u32[i] == expected, "vcmpps predicate %d lane %d: 0x%08x", n, i, dst.u32[i]); \
    } \
} while (0)
    CHECK_PRED(0);  CHECK_PRED(1);  CHECK_PRED(2);  CHECK_PRED(3);
    CHECK_PRED(4);  CHECK_PRED(5);  CHECK_PRED(6);  CHECK_PRED(7);
    CHECK_PRED(8);  CHECK_PRED(9);  CHECK_PRED(10); CHECK_PRED(11);
    CHECK_PRED(12); CHECK_PRED(13); CHECK_PRED(14); CHECK_PRED(15);
    CHECK_PRED(16); CHECK_PRED(17); CHECK_PRED(18); CHECK_PRED(19);
    CHECK_PRED(20); CHECK_PRED(21); CHECK_PRED(22); CHECK_PRED(23);
    CHECK_PRED(24); CHECK_PRED(25); CHECK_PRED(26); CHECK_PRED(27);
    CHECK_PRED(28); CHECK_PRED(29); CHECK_PRED(30); CHECK_PRED(31);
#undef CHECK_PRED

    uint32_t before, after, clean;
    __asm__ volatile ("stmxcsr %0" : "=m"(before));
    clean = before & ~UINT32_C(0x3f);
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean));
    RUN_VCMPPS_PRED(0, dst, a, b); /* quiet predicate with QNaN */
    __asm__ volatile ("stmxcsr %0" : "=m"(after));
    TEST_ASSERT(!(after & 1u), "vcmpps quiet predicate does not signal QNaN invalid");
    __asm__ volatile ("ldmxcsr %0" : : "m"(clean));
    RUN_VCMPPS_PRED(1, dst, a, b); /* signaling predicate */
    __asm__ volatile ("stmxcsr %0" : "=m"(after));
    __asm__ volatile ("ldmxcsr %0" : : "m"(before));
    TEST_ASSERT(after & 1u, "vcmpps signaling predicate sets invalid for QNaN");
}

int main(void) {
    TEST_START("VCMPPS/VCMPPD instructions (AVX)");
    test_vcmpps_eq_256();
    test_vcmpps_lt_256();
    test_vcmppd_eq_256();
    test_vcmpps_neq();
    test_vcmpps_all_predicates();
    __asm__ volatile ("vzeroupper");
    TEST_END();
}
