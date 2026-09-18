/*
 * Test VFMADD132/213/231 PS/PD and scalar SS/SD.
 * The discriminator operands make a separately rounded multiply produce zero,
 * while the fused instruction produces a non-zero exact residual.
 * Compile: gcc -O0 -Wall -Werror -mavx2 -mfma -o test_vfmadd avx256/test_vfmadd.c
 */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_fma(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
    return (ecx >> 12) & 1;
}
#else
#define check_fma() 1
#endif

static void test_pd(void)
{
    TEST_START("VFMADD132/213/231PD fused rounding");
    const double a[4] = {0x1.0000000000001p+0, 2.0, -3.0, 4.0};
    const double b[4] = {-1.0, -2.0, 5.0, -6.0};
    const double c[4] = {0x1.fffffffffffffp-1, 3.0, -7.0, 8.0};
    double r[4];

    __asm__ volatile("vmovupd %1, %%ymm0\n\t"
                     "vmovupd %2, %%ymm1\n\t"
                     "vmovupd %3, %%ymm2\n\t"
                     "vfmadd132pd %%ymm2, %%ymm1, %%ymm0\n\t"
                     "vmovupd %%ymm0, %0"
                     : "=m"(r) : "m"(a), "m"(b), "m"(c) : "ymm0", "ymm1", "ymm2");
    TEST_ASSERT(r[0] == 0x1.ffffffffffffep-54,
                "vfmadd132pd lane0=%a", r[0]);
    TEST_ASSERT(r[1] == a[1] * c[1] + b[1], "vfmadd132pd lane1=%a", r[1]);
    TEST_ASSERT(r[2] == a[2] * c[2] + b[2], "vfmadd132pd lane2=%a", r[2]);
    TEST_ASSERT(r[3] == a[3] * c[3] + b[3], "vfmadd132pd lane3=%a", r[3]);

    __asm__ volatile("vmovupd %1, %%ymm0\n\t"
                     "vmovupd %2, %%ymm1\n\t"
                     "vmovupd %3, %%ymm2\n\t"
                     "vfmadd213pd %%ymm2, %%ymm1, %%ymm0\n\t"
                     "vmovupd %%ymm0, %0"
                     : "=m"(r) : "m"(a), "m"(b), "m"(c) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; ++i)
        TEST_ASSERT(r[i] == b[i] * a[i] + c[i], "vfmadd213pd lane%d=%a", i, r[i]);

    __asm__ volatile("vmovupd %1, %%ymm0\n\t"
                     "vmovupd %2, %%ymm1\n\t"
                     "vmovupd %3, %%ymm2\n\t"
                     "vfmadd231pd %%ymm2, %%ymm1, %%ymm0\n\t"
                     "vmovupd %%ymm0, %0"
                     : "=m"(r) : "m"(a), "m"(b), "m"(c) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; ++i)
        TEST_ASSERT(r[i] == b[i] * c[i] + a[i], "vfmadd231pd lane%d=%a", i, r[i]);

    volatile double rounded_product = a[0] * c[0];
    TEST_ASSERT(rounded_product + b[0] == 0.0,
                "ordinary multiply/add rounds before the add");
}

static void test_ps(void)
{
    TEST_START("VFMADD132/213/231PS fused rounding");
    const float a[8] = {0x1.000002p+0f, 2.0f, -3.0f, 4.0f, 5.0f, -6.0f, 7.0f, -8.0f};
    const float b[8] = {-1.0f, -2.0f, 5.0f, -6.0f, 9.0f, 10.0f, -11.0f, 12.0f};
    const float c[8] = {0x1.fffffep-1f, 3.0f, -7.0f, 8.0f, -9.0f, 10.0f, 11.0f, -12.0f};
    float r[8];

    __asm__ volatile("vmovups %1, %%ymm0\n\t"
                     "vmovups %2, %%ymm1\n\t"
                     "vmovups %3, %%ymm2\n\t"
                     "vfmadd132ps %%ymm2, %%ymm1, %%ymm0\n\t"
                     "vmovups %%ymm0, %0"
                     : "=m"(r) : "m"(a), "m"(b), "m"(c) : "ymm0", "ymm1", "ymm2");
    TEST_ASSERT(r[0] == 0x1.fffffcp-25f, "vfmadd132ps lane0=%a", r[0]);
    for (int i = 1; i < 8; ++i)
        TEST_ASSERT(r[i] == a[i] * c[i] + b[i], "vfmadd132ps lane%d=%a", i, r[i]);

    __asm__ volatile("vmovups %1, %%ymm0\n\t"
                     "vmovups %2, %%ymm1\n\t"
                     "vmovups %3, %%ymm2\n\t"
                     "vfmadd213ps %%ymm2, %%ymm1, %%ymm0\n\t"
                     "vmovups %%ymm0, %0"
                     : "=m"(r) : "m"(a), "m"(b), "m"(c) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; ++i)
        TEST_ASSERT(r[i] == b[i] * a[i] + c[i], "vfmadd213ps lane%d=%a", i, r[i]);

    __asm__ volatile("vmovups %1, %%ymm0\n\t"
                     "vmovups %2, %%ymm1\n\t"
                     "vmovups %3, %%ymm2\n\t"
                     "vfmadd231ps %%ymm2, %%ymm1, %%ymm0\n\t"
                     "vmovups %%ymm0, %0"
                     : "=m"(r) : "m"(a), "m"(b), "m"(c) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; ++i)
        TEST_ASSERT(r[i] == b[i] * c[i] + a[i], "vfmadd231ps lane%d=%a", i, r[i]);

    volatile float rounded_product = a[0] * c[0];
    TEST_ASSERT(rounded_product + b[0] == 0.0f,
                "ordinary single multiply/add rounds before the add");
}

static void test_scalar(void)
{
    TEST_START("VFMADD132/213/231SS/SD");
    double a = 2.0, b = 3.0, c = 4.0, rd;
    float af = 2.0f, bf = 3.0f, cf = 4.0f, rf;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\tvmovsd %3, %%xmm2\n\t"
                     "vfmadd132sd %%xmm2, %%xmm1, %%xmm0\n\tvmovsd %%xmm0, %0"
                     : "=m"(rd) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rd == 11.0, "vfmadd132sd=%a", rd);
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\tvmovsd %3, %%xmm2\n\t"
                     "vfmadd213sd %%xmm2, %%xmm1, %%xmm0\n\tvmovsd %%xmm0, %0"
                     : "=m"(rd) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rd == 10.0, "vfmadd213sd=%a", rd);
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\tvmovsd %3, %%xmm2\n\t"
                     "vfmadd231sd %%xmm2, %%xmm1, %%xmm0\n\tvmovsd %%xmm0, %0"
                     : "=m"(rd) : "m"(a), "m"(b), "m"(c) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rd == 14.0, "vfmadd231sd=%a", rd);
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\tvmovss %3, %%xmm2\n\t"
                     "vfmadd132ss %%xmm2, %%xmm1, %%xmm0\n\tvmovss %%xmm0, %0"
                     : "=m"(rf) : "m"(af), "m"(bf), "m"(cf) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rf == 11.0f, "vfmadd132ss=%a", rf);
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\tvmovss %3, %%xmm2\n\t"
                     "vfmadd213ss %%xmm2, %%xmm1, %%xmm0\n\tvmovss %%xmm0, %0"
                     : "=m"(rf) : "m"(af), "m"(bf), "m"(cf) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rf == 10.0f, "vfmadd213ss=%a", rf);
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\tvmovss %3, %%xmm2\n\t"
                     "vfmadd231ss %%xmm2, %%xmm1, %%xmm0\n\tvmovss %%xmm0, %0"
                     : "=m"(rf) : "m"(af), "m"(bf), "m"(cf) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rf == 14.0f, "vfmadd231ss=%a", rf);
}

static void test_rounding_discriminator(void)
{
    TEST_START("VFMADD fused-vs-separate rounding");
    const double x = 0x1.0000000000001p+0;
    const double y = 0x1.fffffffffffffp-1;
    const double z = -1.0;
    double r;

    /* For these exact operands, separately rounded x*y followed by +z is
     * known to produce 0.0. Keep that result as a fixed oracle rather than
     * recomputing it with the compiler under test. */
    const double separate = 0.0;
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\tvmovsd %3, %%xmm2\n\t"
                     "vfmadd132sd %%xmm2, %%xmm1, %%xmm0\n\tvmovsd %%xmm0, %0"
                     : "=m"(r) : "m"(x), "m"(z), "m"(y) : "xmm0", "xmm1", "xmm2");
    printf("vfmadd132sd fused=%a separate=%a", r, separate);
    TEST_ASSERT(r != separate, "vfmadd132sd fused=%a separate=%a", r, separate);
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\tvmovsd %3, %%xmm2\n\t"
                     "vfmadd213sd %%xmm2, %%xmm1, %%xmm0\n\tvmovsd %%xmm0, %0"
                     : "=m"(r) : "m"(y), "m"(x), "m"(z) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(r != separate, "vfmadd213sd fused=%a separate=%a", r, separate);
    __asm__ volatile("vmovsd %1, %%xmm0\n\tvmovsd %2, %%xmm1\n\tvmovsd %3, %%xmm2\n\t"
                     "vfmadd231sd %%xmm2, %%xmm1, %%xmm0\n\tvmovsd %%xmm0, %0"
                     : "=m"(r) : "m"(z), "m"(x), "m"(y) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(r != separate, "vfmadd231sd fused=%a separate=%a", r, separate);

    const float xf = 0x1.000002p+0f;
    const float yf = 0x1.fffffep-1f;
    const float zf = -1.0f;
    float rf;
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\tvmovss %3, %%xmm2\n\t"
                     "vfmadd132ss %%xmm2, %%xmm1, %%xmm0\n\tvmovss %%xmm0, %0"
                     : "=m"(rf) : "m"(xf), "m"(zf), "m"(yf) : "xmm0", "xmm1", "xmm2");
    /* The separately rounded single-precision result is also exactly zero. */
    const float separatef = 0.0f;
    TEST_ASSERT(rf != separatef, "vfmadd132ss fused=%a separate=%a", rf, separatef);
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\tvmovss %3, %%xmm2\n\t"
                     "vfmadd213ss %%xmm2, %%xmm1, %%xmm0\n\tvmovss %%xmm0, %0"
                     : "=m"(rf) : "m"(yf), "m"(xf), "m"(zf) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rf != separatef, "vfmadd213ss fused=%a separate=%a", rf, separatef);
    __asm__ volatile("vmovss %1, %%xmm0\n\tvmovss %2, %%xmm1\n\tvmovss %3, %%xmm2\n\t"
                     "vfmadd231ss %%xmm2, %%xmm1, %%xmm0\n\tvmovss %%xmm0, %0"
                     : "=m"(rf) : "m"(zf), "m"(xf), "m"(yf) : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(rf != separatef, "vfmadd231ss fused=%a separate=%a", rf, separatef);
}

int main(void)
{
    if (!check_fma()) {
        printf("FMA not supported\n");
        return 1;
    }
    test_pd();
    test_ps();
    test_scalar();
    test_rounding_discriminator();
    TEST_END();
}
