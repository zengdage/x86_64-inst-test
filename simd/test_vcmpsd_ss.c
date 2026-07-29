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
    default: return -1;
    }
    memcpy(&r, &vr.u32[0], 4);
    return r ? 1 : 0;
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

    TEST_END();
}
