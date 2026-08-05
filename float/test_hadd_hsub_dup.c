/*
 * test_hadd_hsub_dup.c - Test SSE3 horizontal add/sub, addsub, and dup instructions
 *
 * HADDPS  : Horizontal add adjacent pairs of single-precision floats.
 * HADDPD  : Horizontal add adjacent pairs of double-precision floats.
 * HSUBPS  : Horizontal sub adjacent pairs of single-precision floats.
 * HSUBPD  : Horizontal sub adjacent pairs of double-precision floats.
 * ADDSUBPS: Alternating sub/add on packed single-precision floats (even sub, odd add).
 * ADDSUBPD: Alternating sub/add on packed double-precision floats (even sub, odd add).
 * MOVDDUP : Duplicate double across both qwords of an xmm.
 * MOVSHDUP: Shuffle odd-indexed single-precision floats (1,3) to (0,1,2,3).
 * MOVSLDUP: Shuffle even-indexed single-precision floats (0,2) to (0,1,2,3).
 *
 * Compile: gcc -o test_hadd_hsub_dup float/test_hadd_hsub_dup.c -O2 -msse3 -lm
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <math.h>
#include <float.h>

static int sse3_available(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
    return (ecx >> 0) & 1;
}

static void test_haddps_basic(void) {
    xmm_t a, r;
    a.f32[0] = 1.0f; a.f32[1] = 2.0f; a.f32[2] = 3.0f; a.f32[3] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "haddps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    /* haddps(x,x): [x0+x1, x2+x3, x0+x1, x2+x3] */
    TEST_ASSERT(r.f32[0] == 3.0f, "haddps [0] 1+2=3: got %f", r.f32[0]);
    TEST_ASSERT(r.f32[1] == 7.0f, "haddps [1] 3+4=7: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 3.0f, "haddps [2] 1+2=3: got %f", r.f32[2]);
    TEST_ASSERT(r.f32[3] == 7.0f, "haddps [3] 3+4=7: got %f", r.f32[3]);
}

static void test_haddps_two_op(void) {
    xmm_t a, b, r;
    a.f32[0] = 10.0f; a.f32[1] = 20.0f; a.f32[2] = 30.0f; a.f32[3] = 40.0f;
    b.f32[0] = 1.0f;  b.f32[1] = 2.0f;  b.f32[2] = 3.0f;  b.f32[3] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "haddps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    /* haddps(a,b): [a0+a1, a2+a3, b0+b1, b2+b3] */
    TEST_ASSERT(r.f32[0] == 30.0f, "haddps 2op [0] 10+20=30: got %f", r.f32[0]);
    TEST_ASSERT(r.f32[1] == 70.0f, "haddps 2op [1] 30+40=70: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 3.0f,  "haddps 2op [2] 1+2=3: got %f",   r.f32[2]);
    TEST_ASSERT(r.f32[3] == 7.0f,  "haddps 2op [3] 3+4=7: got %f",   r.f32[3]);
}

static void test_haddpd_basic(void) {
    xmm_t a, r;
    a.f64[0] = 1.5; a.f64[1] = 2.5;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "haddpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    /* haddpd(x,x): [x0+x1, x0+x1] */
    TEST_ASSERT(r.f64[0] == 4.0, "haddpd [0] 1.5+2.5=4.0: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 4.0, "haddpd [1] 1.5+2.5=4.0: got %f", r.f64[1]);
}

static void test_hsubps_basic(void) {
    xmm_t a, r;
    a.f32[0] = 10.0f; a.f32[1] = 3.0f; a.f32[2] = 20.0f; a.f32[3] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "hsubps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    /* hsubps(x,x): [x0-x1, x2-x3, x0-x1, x2-x3] */
    TEST_ASSERT(r.f32[0] == 7.0f,  "hsubps [0] 10-3=7: got %f",  r.f32[0]);
    TEST_ASSERT(r.f32[1] == 15.0f, "hsubps [1] 20-5=15: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 7.0f,  "hsubps [2] 10-3=7: got %f",  r.f32[2]);
    TEST_ASSERT(r.f32[3] == 15.0f, "hsubps [3] 20-5=15: got %f", r.f32[3]);
}

static void test_hsubpd_basic(void) {
    xmm_t a, r;
    a.f64[0] = 10.0; a.f64[1] = 3.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "hsubpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 7.0, "hsubpd [0] 10-3=7: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 7.0, "hsubpd [1] 10-3=7: got %f", r.f64[1]);
}

static void test_addsubps_basic(void) {
    xmm_t a, b, r;
    a.f32[0] = 10.0f; a.f32[1] = 20.0f; a.f32[2] = 30.0f; a.f32[3] = 40.0f;
    b.f32[0] = 1.0f;  b.f32[1] = 2.0f;  b.f32[2] = 3.0f;  b.f32[3] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addsubps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    /* even indices (0,2): sub; odd indices (1,3): add */
    TEST_ASSERT(r.f32[0] == 9.0f,  "addsubps [0] 10-1=9: got %f",  r.f32[0]);
    TEST_ASSERT(r.f32[1] == 22.0f, "addsubps [1] 20+2=22: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 27.0f, "addsubps [2] 30-3=27: got %f", r.f32[2]);
    TEST_ASSERT(r.f32[3] == 44.0f, "addsubps [3] 40+4=44: got %f", r.f32[3]);
}

static void test_addsubpd_basic(void) {
    xmm_t a, b, r;
    a.f64[0] = 10.0; a.f64[1] = 20.0;
    b.f64[0] = 1.0;  b.f64[1] = 2.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsubpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    /* even index (0): sub; odd index (1): add */
    TEST_ASSERT(r.f64[0] == 9.0,  "addsubpd [0] 10-1=9: got %f",  r.f64[0]);
    TEST_ASSERT(r.f64[1] == 22.0, "addsubpd [1] 20+2=22: got %f", r.f64[1]);
}

static void test_addsubps_mem(void) {
    xmm_t a, b, r;
    a.f32[0] = 100.0f; a.f32[1] = 200.0f; a.f32[2] = 300.0f; a.f32[3] = 400.0f;
    b.f32[0] = 1.0f;   b.f32[1] = 2.0f;   b.f32[2] = 3.0f;   b.f32[3] = 4.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "addsubps %2, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0"
    );
    TEST_ASSERT(r.f32[0] == 99.0f,  "addsubps mem [0] 100-1=99: got %f",   r.f32[0]);
    TEST_ASSERT(r.f32[1] == 202.0f, "addsubps mem [1] 200+2=202: got %f",  r.f32[1]);
    TEST_ASSERT(r.f32[2] == 297.0f, "addsubps mem [2] 300-3=297: got %f",  r.f32[2]);
    TEST_ASSERT(r.f32[3] == 404.0f, "addsubps mem [3] 400+4=404: got %f",  r.f32[3]);
}

static void test_movddup_basic(void) {
    xmm_t a, r;
    a.f64[0] = 3.14; a.f64[1] = 99.0;
    __asm__ volatile (
        "movddup %1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 3.14, "movddup [0] duplicated: got %f", r.f64[0]);
    TEST_ASSERT(r.f64[1] == 3.14, "movddup [1] duplicated: got %f", r.f64[1]);
}

static void test_movshdup_basic(void) {
    xmm_t a, r;
    a.f32[0] = 1.0f; a.f32[1] = 2.0f; a.f32[2] = 3.0f; a.f32[3] = 4.0f;
    __asm__ volatile (
        "movshdup %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    /* movshdup: [a1, a1, a3, a3] */
    TEST_ASSERT(r.f32[0] == 2.0f, "movshdup [0]=a1: got %f", r.f32[0]);
    TEST_ASSERT(r.f32[1] == 2.0f, "movshdup [1]=a1: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 4.0f, "movshdup [2]=a3: got %f", r.f32[2]);
    TEST_ASSERT(r.f32[3] == 4.0f, "movshdup [3]=a3: got %f", r.f32[3]);
}

static void test_movsldup_basic(void) {
    xmm_t a, r;
    a.f32[0] = 1.0f; a.f32[1] = 2.0f; a.f32[2] = 3.0f; a.f32[3] = 4.0f;
    __asm__ volatile (
        "movsldup %1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    /* movsldup: [a0, a0, a2, a2] */
    TEST_ASSERT(r.f32[0] == 1.0f, "movsldup [0]=a0: got %f", r.f32[0]);
    TEST_ASSERT(r.f32[1] == 1.0f, "movsldup [1]=a0: got %f", r.f32[1]);
    TEST_ASSERT(r.f32[2] == 3.0f, "movsldup [2]=a2: got %f", r.f32[2]);
    TEST_ASSERT(r.f32[3] == 3.0f, "movsldup [3]=a2: got %f", r.f32[3]);
}

static void test_haddps_special(void) {
    xmm_t a, r;
    a.f32[0] = INFINITY; a.f32[1] = -INFINITY; a.f32[2] = NAN; a.f32[3] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "haddps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(IS_QNAN(r.f32[0]), "haddps inf+(-inf)=QNaN [0]");
    TEST_ASSERT(IS_QNAN(r.f32[1]), "haddps QNaN+1=QNaN [1]");
    TEST_ASSERT(IS_QNAN(r.f32[2]), "haddps inf+(-inf)=QNaN [2]");
    TEST_ASSERT(IS_QNAN(r.f32[3]), "haddps QNaN+1=QNaN [3]");
}

static void test_haddps_zero(void) {
    xmm_t a, r;
    a.f32[0] = 0.0f; a.f32[1] = 0.0f; a.f32[2] = -0.0f; a.f32[3] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "haddps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r.f32[0] == 0.0f, "haddps 0+0=0 [0]");
    TEST_ASSERT(!signbit(r.f32[1]), "haddps -0+0=-0 [1]");
    TEST_ASSERT(r.f32[2] == 0.0f, "haddps 0+0=0 [2]");
    TEST_ASSERT(!signbit(r.f32[3]), "haddps -0+0=-0 [3]");
}

static void test_haddps_inf(void) {
    xmm_t a, r;
    a.f32[0] = INFINITY; a.f32[1] = 1.0f; a.f32[2] = INFINITY; a.f32[3] = -INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "haddps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(isinf(r.f32[0]) && r.f32[0] > 0, "haddps inf+1=inf [0]");
    TEST_ASSERT(IS_QNAN(r.f32[1]), "haddps inf+(-inf)=QNaN [1]");
    TEST_ASSERT(isinf(r.f32[2]) && r.f32[2] > 0, "haddps inf+1=inf [2]");
    TEST_ASSERT(IS_QNAN(r.f32[3]), "haddps inf+(-inf)=QNaN [3]");
}

static void test_haddpd_special(void) {
    xmm_t a, r;
    a.f64[0] = INFINITY; a.f64[1] = -INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "haddpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(IS_QNAN(r.f64[0]), "haddpd inf+(-inf)=QNaN [0]");
    TEST_ASSERT(IS_QNAN(r.f64[1]), "haddpd inf+(-inf)=QNaN [1]");
}

static void test_haddpd_zero(void) {
    xmm_t a, r;
    a.f64[0] = 0.0; a.f64[1] = -0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "haddpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 0.0, "haddpd 0+(-0)=0 [0]");
    TEST_ASSERT(r.f64[1] == 0.0, "haddpd 0+(-0)=0 [1]");
}

static void test_haddpd_inf(void) {
    xmm_t a, r;
    a.f64[0] = INFINITY; a.f64[1] = 100.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "haddpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(isinf(r.f64[0]) && r.f64[0] > 0, "haddpd inf+100=inf [0]");
    TEST_ASSERT(isinf(r.f64[1]) && r.f64[1] > 0, "haddpd inf+100=inf [1]");
}

static void test_hsubps_special(void) {
    xmm_t a, r;
    a.f32[0] = INFINITY; a.f32[1] = -INFINITY; a.f32[2] = NAN; a.f32[3] = 1.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "hsubps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(isinf(r.f32[0]) && r.f32[0] > 0, "hsubps inf-(-inf)=inf [0]");
    TEST_ASSERT(IS_QNAN(r.f32[1]), "hsubps QNaN-1=QNaN [1]");
    TEST_ASSERT(isinf(r.f32[2]) && r.f32[2] > 0, "hsubps inf-(-inf)=inf [2]");
    TEST_ASSERT(IS_QNAN(r.f32[3]), "hsubps QNaN-1=QNaN [3]");
}

static void test_hsubps_inf_nan(void) {
    xmm_t a, r;
    a.f32[0] = INFINITY; a.f32[1] = INFINITY; a.f32[2] = 5.0f; a.f32[3] = 5.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "hsubps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(IS_QNAN(r.f32[0]), "hsubps inf-inf=QNaN [0]");
    TEST_ASSERT(r.f32[1] == 0.0f, "hsubps 5-5=0 [1]");
    TEST_ASSERT(IS_QNAN(r.f32[2]), "hsubps inf-inf=QNaN [2]");
    TEST_ASSERT(r.f32[3] == 0.0f, "hsubps 5-5=0 [3]");
}

static void test_hsubps_zero(void) {
    xmm_t a, r;
    a.f32[0] = 0.0f; a.f32[1] = 0.0f; a.f32[2] = 0.0f; a.f32[3] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "hsubps %%xmm0, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r.f32[0] == 0.0f, "hsubps 0-0=0 [0]");
    TEST_ASSERT(r.f32[1] == 0.0f, "hsubps 0-0=0 [1]");
    TEST_ASSERT(r.f32[2] == 0.0f, "hsubps 0-0=0 [2]");
    TEST_ASSERT(r.f32[3] == 0.0f, "hsubps 0-0=0 [3]");
}

static void test_hsubpd_special(void) {
    xmm_t a, r;
    a.f64[0] = INFINITY; a.f64[1] = -INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "hsubpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(isinf(r.f64[0]) && r.f64[0] > 0, "hsubpd inf-(-inf)=inf [0]");
    TEST_ASSERT(isinf(r.f64[1]) && r.f64[1] > 0, "hsubpd inf-(-inf)=inf [1]");
}

static void test_hsubpd_inf_nan(void) {
    xmm_t a, r;
    a.f64[0] = INFINITY; a.f64[1] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "hsubpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(IS_QNAN(r.f64[0]), "hsubpd inf-inf=QNaN [0]");
    TEST_ASSERT(IS_QNAN(r.f64[1]), "hsubpd inf-inf=QNaN [1]");
}

static void test_hsubpd_zero(void) {
    xmm_t a, r;
    a.f64[0] = 0.0; a.f64[1] = 0.0;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "hsubpd %%xmm0, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a) : "xmm0"
    );
    TEST_ASSERT(r.f64[0] == 0.0, "hsubpd 0-0=0 [0]");
    TEST_ASSERT(r.f64[1] == 0.0, "hsubpd 0-0=0 [1]");
}

static void test_addsubps_special(void) {
    xmm_t a, b, r;
    a.f32[0] = INFINITY; a.f32[1] = -INFINITY; a.f32[2] = NAN; a.f32[3] = 0.0f;
    b.f32[0] = 1.0f;      b.f32[1] = -INFINITY;  b.f32[2] = 1.0f;  b.f32[3] = 0.0f;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addsubps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(r.f32[0]) && r.f32[0] > 0, "addsubps [0] inf-1=inf");
    TEST_ASSERT(isinf(r.f32[1]) && r.f32[1] < 0, "addsubps [1] -inf+(-inf)=-inf");
    TEST_ASSERT(IS_QNAN(r.f32[2]), "addsubps [2] QNaN-1=QNaN");
    TEST_ASSERT(r.f32[3] == 0.0f, "addsubps [3] 0+0=0");
}

static void test_addsubps_inf_nan(void) {
    xmm_t a, b, r;
    a.f32[0] = INFINITY; a.f32[1] = INFINITY; a.f32[2] = INFINITY; a.f32[3] = INFINITY;
    b.f32[0] = INFINITY; b.f32[1] = INFINITY; b.f32[2] = INFINITY; b.f32[3] = INFINITY;
    __asm__ volatile (
        "movaps %1, %%xmm0\n\t"
        "movaps %2, %%xmm1\n\t"
        "addsubps %%xmm1, %%xmm0\n\t"
        "movaps %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(r.f32[0]), "addsubps [0] inf-inf=QNaN");
    TEST_ASSERT(isinf(r.f32[1]) && r.f32[1] > 0, "addsubps [1] inf+inf=inf");
    TEST_ASSERT(IS_QNAN(r.f32[2]), "addsubps [2] inf-inf=QNaN");
    TEST_ASSERT(isinf(r.f32[3]) && r.f32[3] > 0, "addsubps [3] inf+inf=inf");
}

static void test_addsubpd_special(void) {
    xmm_t a, b, r;
    a.f64[0] = INFINITY; a.f64[1] = -INFINITY;
    b.f64[0] = 1.0;      b.f64[1] = -INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsubpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    TEST_ASSERT(isinf(r.f64[0]) && r.f64[0] > 0, "addsubpd [0] inf-1=inf");
    TEST_ASSERT(isinf(r.f64[1]) && r.f64[1] < 0, "addsubpd [1] -inf+(-inf)=-inf");
}

static void test_addsubpd_inf_nan(void) {
    xmm_t a, b, r;
    a.f64[0] = INFINITY; a.f64[1] = INFINITY;
    b.f64[0] = INFINITY; b.f64[1] = INFINITY;
    __asm__ volatile (
        "movapd %1, %%xmm0\n\t"
        "movapd %2, %%xmm1\n\t"
        "addsubpd %%xmm1, %%xmm0\n\t"
        "movapd %%xmm0, %0"
        : "=m"(r) : "m"(a), "m"(b) : "xmm0", "xmm1"
    );
    TEST_ASSERT(IS_QNAN(r.f64[0]), "addsubpd [0] inf-inf=QNaN");
    TEST_ASSERT(isinf(r.f64[1]) && r.f64[1] > 0, "addsubpd [1] inf+inf=inf");
}

int main(void) {
    if (!sse3_available()) {
        printf("SSE3 not supported, skipping\n");
        return 0;
    }
    TEST_START("HADDPS/HADDPD/HSUBPS/HSUBPD/ADDSUBPS/ADDSUBPD/MOVDDUP/MOVSHDUP/MOVSLDUP");
    test_haddps_basic();
    test_haddps_two_op();
    test_haddps_special();
    test_haddps_zero();
    test_haddps_inf();
    test_haddpd_basic();
    test_haddpd_special();
    test_haddpd_zero();
    test_haddpd_inf();
    test_hsubps_basic();
    test_hsubps_special();
    test_hsubps_inf_nan();
    test_hsubps_zero();
    test_hsubpd_basic();
    test_hsubpd_special();
    test_hsubpd_inf_nan();
    test_hsubpd_zero();
    test_addsubps_basic();
    test_addsubpd_basic();
    test_addsubps_mem();
    test_addsubps_special();
    test_addsubps_inf_nan();
    test_addsubpd_special();
    test_addsubpd_inf_nan();
    test_movddup_basic();
    test_movshdup_basic();
    test_movsldup_basic();
    TEST_END();
}
