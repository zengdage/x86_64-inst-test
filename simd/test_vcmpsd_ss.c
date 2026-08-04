#include "../common.h"
#include <immintrin.h>
#include <math.h>

/* VCMPSD/VCMPSS predicates */
#define CMP_EQ_OQ   0
#define CMP_LT_OS   1
#define CMP_LE_OS   2
#define CMP_UNORD_Q 3
#define CMP_NEQ_UQ  4
#define CMP_NLT_US  5
#define CMP_NLE_US  6
#define CMP_ORD_Q   7

#define VCMP_CASE_SD(n) case n: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $" #n ",%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break
#define VCMP_CASE_SS(n) case n: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $" #n ",%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break

static int vcmpsd(double a, double b, int pred) {
    uint64_t r;
    xmm_t va, vb, vr;
    va.f64[0] = a; va.f64[1] = 0;
    vb.f64[0] = b; vb.f64[1] = 0;
    switch (pred) {
    case 0: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $0,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 1: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $1,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 2: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $2,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 3: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $3,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 4: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $4,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 5: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $5,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 6: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $6,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 7: __asm__("vmovapd %1,%%xmm0\n\t vmovapd %2,%%xmm1\n\t vcmpsd $7,%%xmm1,%%xmm0,%%xmm2\n\t vmovapd %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    VCMP_CASE_SD(8);  VCMP_CASE_SD(9);  VCMP_CASE_SD(10); VCMP_CASE_SD(11);
    VCMP_CASE_SD(12); VCMP_CASE_SD(13); VCMP_CASE_SD(14); VCMP_CASE_SD(15);
    VCMP_CASE_SD(16); VCMP_CASE_SD(17); VCMP_CASE_SD(18); VCMP_CASE_SD(19);
    VCMP_CASE_SD(20); VCMP_CASE_SD(21); VCMP_CASE_SD(22); VCMP_CASE_SD(23);
    VCMP_CASE_SD(24); VCMP_CASE_SD(25); VCMP_CASE_SD(26); VCMP_CASE_SD(27);
    VCMP_CASE_SD(28); VCMP_CASE_SD(29); VCMP_CASE_SD(30); VCMP_CASE_SD(31);
    default: return -1;
    }
    memcpy(&r, &vr.u64[0], 8);
    return r ? 1 : 0;
}

static int vcmpss(float a, float b, int pred) {
    uint32_t r;
    xmm_t va, vb, vr;
    va.f32[0] = a; va.f32[1] = va.f32[2] = va.f32[3] = 0;
    vb.f32[0] = b; vb.f32[1] = vb.f32[2] = vb.f32[3] = 0;
    switch (pred) {
    case 0: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $0,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 1: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $1,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 2: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $2,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 3: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $3,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 4: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $4,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 5: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $5,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 6: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $6,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    case 7: __asm__("vmovaps %1,%%xmm0\n\t vmovaps %2,%%xmm1\n\t vcmpss $7,%%xmm1,%%xmm0,%%xmm2\n\t vmovaps %%xmm2,%0":"=m"(vr):"m"(va),"m"(vb):"xmm0","xmm1","xmm2"); break;
    VCMP_CASE_SS(8);  VCMP_CASE_SS(9);  VCMP_CASE_SS(10); VCMP_CASE_SS(11);
    VCMP_CASE_SS(12); VCMP_CASE_SS(13); VCMP_CASE_SS(14); VCMP_CASE_SS(15);
    VCMP_CASE_SS(16); VCMP_CASE_SS(17); VCMP_CASE_SS(18); VCMP_CASE_SS(19);
    VCMP_CASE_SS(20); VCMP_CASE_SS(21); VCMP_CASE_SS(22); VCMP_CASE_SS(23);
    VCMP_CASE_SS(24); VCMP_CASE_SS(25); VCMP_CASE_SS(26); VCMP_CASE_SS(27);
    VCMP_CASE_SS(28); VCMP_CASE_SS(29); VCMP_CASE_SS(30); VCMP_CASE_SS(31);
    default: return -1;
    }
    memcpy(&r, &vr.u32[0], 4);
    return r ? 1 : 0;
}

static int scalar_predicate_expected(double a, double b, int pred) {
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

static void test_scalar_compare_merge_zeroing_and_exceptions(void) {
    ymm_t initial, result;
    xmm_t a = { .u64 = {UINT64_C(0x3ff0000000000000),
                         UINT64_C(0x7ff8123456789abc)} };
    xmm_t b = { .u64 = {UINT64_C(0x3ff0000000000000),
                         UINT64_C(0x1111222233334444)} };
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile(
        "vmovdqu %1,%%ymm2\n\tvmovdqu %2,%%xmm0\n\t"
        "vcmpsd $0,%3,%%xmm0,%%xmm2\n\tvmovdqu %%ymm2,%0"
        : "=m"(result) : "m"(initial), "m"(a), "m"(b)
        : "xmm0", "ymm2");
    TEST_ASSERT(result.u64[0] == UINT64_MAX, "vcmpsd true result all ones");
    TEST_ASSERT(result.u64[1] == a.u64[1], "vcmpsd upper qword from first source");
    TEST_ASSERT(result.u64[2] == 0 && result.u64[3] == 0, "vcmpsd clears upper YMM");

    xmm_t af = { .u32 = {UINT32_C(0x3f800000), UINT32_C(0x80000000),
                          UINT32_C(0x7fc12345), UINT32_C(0x00000001)} };
    xmm_t bf = { .u32 = {UINT32_C(0x40000000), 0, 0, 0} };
    __asm__ volatile(
        "vmovdqu %1,%%ymm2\n\tvmovdqu %2,%%xmm0\n\t"
        "vcmpss $1,%3,%%xmm0,%%xmm2\n\tvmovdqu %%ymm2,%0"
        : "=m"(result) : "m"(initial), "m"(af), "m"(bf)
        : "xmm0", "ymm2");
    TEST_ASSERT(result.u32[0] == UINT32_MAX, "vcmpss true result all ones");
    for (int lane = 1; lane < 4; lane++)
        TEST_ASSERT(result.u32[lane] == af.u32[lane],
                    "vcmpss upper XMM lane %d from first source", lane);
    for (int lane = 4; lane < 8; lane++)
        TEST_ASSERT(result.u32[lane] == 0, "vcmpss clears upper YMM lane %d", lane);

    xmm_t qnan = { .u32 = {UINT32_C(0x7fc12345), 0, 0, 0} };
    uint32_t saved, csr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("vmovdqu %0,%%xmm0\n\tvcmpss $0,%1,%%xmm0,%%xmm1"
        : : "m"(qnan), "m"(bf) : "xmm0", "xmm1");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(!(csr & 1u), "vcmpss quiet predicate does not signal QNaN invalid");
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("vmovdqu %0,%%xmm0\n\tvcmpss $1,%1,%%xmm0,%%xmm1"
        : : "m"(qnan), "m"(bf) : "xmm0", "xmm1");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(csr & 1u, "vcmpss signaling predicate sets QNaN invalid");
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
}

int main(void) {
    TEST_START("VCMPSD/VCMPSS");

    double nan_d = __builtin_nan("");
    float  nan_f = __builtin_nanf("");

    /* VCMPSD: a=1.0, b=2.0 => a < b */
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_EQ_OQ)   == 0, "vcmpsd EQ 1<2");
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_LT_OS)   == 1, "vcmpsd LT 1<2");
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_LE_OS)   == 1, "vcmpsd LE 1<2");
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_UNORD_Q) == 0, "vcmpsd UNORD 1<2");
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_NEQ_UQ)  == 1, "vcmpsd NEQ 1<2");
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_NLT_US)  == 0, "vcmpsd NLT 1<2");
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_NLE_US)  == 0, "vcmpsd NLE 1<2");
    TEST_ASSERT(vcmpsd(1.0, 2.0, CMP_ORD_Q)   == 1, "vcmpsd ORD 1<2");

    /* VCMPSD: a==b */
    TEST_ASSERT(vcmpsd(3.0, 3.0, CMP_EQ_OQ)   == 1, "vcmpsd EQ 3==3");
    TEST_ASSERT(vcmpsd(3.0, 3.0, CMP_LT_OS)   == 0, "vcmpsd LT 3==3");
    TEST_ASSERT(vcmpsd(3.0, 3.0, CMP_LE_OS)   == 1, "vcmpsd LE 3==3");
    TEST_ASSERT(vcmpsd(3.0, 3.0, CMP_NEQ_UQ)  == 0, "vcmpsd NEQ 3==3");
    TEST_ASSERT(vcmpsd(3.0, 3.0, CMP_NLT_US)  == 1, "vcmpsd NLT 3==3");
    TEST_ASSERT(vcmpsd(3.0, 3.0, CMP_NLE_US)  == 0, "vcmpsd NLE 3==3");

    /* VCMPSD with NaN: UNORD=1, ORD=0, EQ_OQ=0, NEQ_UQ=1 */
    TEST_ASSERT(vcmpsd(nan_d, 1.0, CMP_UNORD_Q) == 1, "vcmpsd UNORD nan");
    TEST_ASSERT(vcmpsd(nan_d, 1.0, CMP_ORD_Q)   == 0, "vcmpsd ORD nan");
    TEST_ASSERT(vcmpsd(nan_d, 1.0, CMP_EQ_OQ)   == 0, "vcmpsd EQ nan");
    TEST_ASSERT(vcmpsd(nan_d, 1.0, CMP_NEQ_UQ)  == 1, "vcmpsd NEQ nan");
    TEST_ASSERT(vcmpsd(nan_d, 1.0, CMP_LT_OS)   == 0, "vcmpsd LT nan");
    TEST_ASSERT(vcmpsd(nan_d, 1.0, CMP_NLT_US)  == 1, "vcmpsd NLT nan");

    /* VCMPSS: a=5.0, b=3.0 => a > b */
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_EQ_OQ)   == 0, "vcmpss EQ 5>3");
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_LT_OS)   == 0, "vcmpss LT 5>3");
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_LE_OS)   == 0, "vcmpss LE 5>3");
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_UNORD_Q) == 0, "vcmpss UNORD 5>3");
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_NEQ_UQ)  == 1, "vcmpss NEQ 5>3");
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_NLT_US)  == 1, "vcmpss NLT 5>3");
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_NLE_US)  == 1, "vcmpss NLE 5>3");
    TEST_ASSERT(vcmpss(5.0f, 3.0f, CMP_ORD_Q)   == 1, "vcmpss ORD 5>3");

    /* VCMPSS with NaN */
    TEST_ASSERT(vcmpss(nan_f, 1.0f, CMP_UNORD_Q) == 1, "vcmpss UNORD nan");
    TEST_ASSERT(vcmpss(nan_f, 1.0f, CMP_ORD_Q)   == 0, "vcmpss ORD nan");
    TEST_ASSERT(vcmpss(nan_f, 1.0f, CMP_EQ_OQ)   == 0, "vcmpss EQ nan");
    TEST_ASSERT(vcmpss(nan_f, 1.0f, CMP_NEQ_UQ)  == 1, "vcmpss NEQ nan");

    for (int pred = 0; pred < 32; pred++) {
        TEST_ASSERT(vcmpsd(nan_d, 1.0, pred) == scalar_predicate_expected(nan_d, 1.0, pred),
            "vcmpsd predicate %d NaN truth table", pred);
        TEST_ASSERT(vcmpss(2.0f, 1.0f, pred) == scalar_predicate_expected(2.0, 1.0, pred),
            "vcmpss predicate %d greater truth table", pred);
    }

    test_scalar_compare_merge_zeroing_and_exceptions();

    TEST_END();
}
