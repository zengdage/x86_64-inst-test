/*
 * Test VDPPD/VDPPPS
 * VDPPD: 128-bit xmm dot product of packed doubles (imm8 controls input/output mask)
 * VDPPPS: 256-bit ymm dot product of packed floats
 * Compile: gcc -o test_vdppd_dpps avx256/test_vdppd_dpps.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(1),"c"(0));
    return (ecx >> 28) & 1;
}
#else
#define check_avx() 1
#endif

static void test_vdppd(void) {
    TEST_START("VDPPD (128-bit xmm dot product of doubles)");
    /* imm8: bits[5:4] = input mask (which elements to multiply), bits[1:0] = output mask */
    /* imm8=0x31: input mask=0b11 (both), output mask=0b01 (lane0 only) */
    double a[2] = {2.0, 3.0};
    double b[2] = {4.0, 5.0};
    double r[2];
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovupd %2, %%xmm1\n\t"
        "vdppd $0x31, %%xmm1, %%xmm0, %%xmm2\n\t"  /* dot=2*4+3*5=23, out to lane0 */
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r[0] == 23.0, "vdppd 0x31 r[0]=%f (2*4+3*5=23)", r[0]);
    TEST_ASSERT(r[1] == 0.0,  "vdppd 0x31 r[1]=%f (output mask bit1=0)", r[1]);

    /* imm8=0x33: output to both lanes */
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovupd %2, %%xmm1\n\t"
        "vdppd $0x33, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r[0] == 23.0, "vdppd 0x33 r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 23.0, "vdppd 0x33 r[1]=%f", r[1]);

    /* imm8=0x11: only lane0 input, lane0 output: 2*4=8 */
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vmovupd %2, %%xmm1\n\t"
        "vdppd $0x11, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm1","xmm2"
    );
    TEST_ASSERT(r[0] == 8.0, "vdppd 0x11 r[0]=%f (2*4=8)", r[0]);

    /* mem operand form */
    __asm__ volatile(
        "vmovupd %1, %%xmm0\n\t"
        "vdppd $0x31, %2, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "xmm0","xmm2"
    );
    TEST_ASSERT(r[0] == 23.0, "vdppd mem r[0]=%f", r[0]);
}

static void test_vdppps(void) {
    TEST_START("VDPPPS (256-bit ymm dot product of floats)");
    /* 256-bit: operates on two independent 128-bit halves */
    /* imm8=0xFF: all inputs, all outputs */
    float a[8] = {1,2,3,4, 1,2,3,4};
    float b[8] = {5,6,7,8, 5,6,7,8};
    float r[8];
    /* dot per 128-bit half: 1*5+2*6+3*7+4*8 = 5+12+21+32 = 70 */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vdpps $0xFF, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 70.0f, "vdpps 0xFF r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 70.0f, "vdpps 0xFF r[1]=%f", r[1]);
    TEST_ASSERT(r[2] == 70.0f, "vdpps 0xFF r[2]=%f", r[2]);
    TEST_ASSERT(r[3] == 70.0f, "vdpps 0xFF r[3]=%f", r[3]);
    TEST_ASSERT(r[4] == 70.0f, "vdpps 0xFF r[4]=%f (upper half)", r[4]);

    /* imm8=0xF1: all inputs, only lane0 of each 128-bit half */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vmovups %2, %%ymm1\n\t"
        "vdpps $0xF1, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm1","ymm2"
    );
    TEST_ASSERT(r[0] == 70.0f, "vdpps 0xF1 r[0]=%f", r[0]);
    TEST_ASSERT(r[1] == 0.0f,  "vdpps 0xF1 r[1]=%f (output mask)", r[1]);
    TEST_ASSERT(r[4] == 70.0f, "vdpps 0xF1 r[4]=%f (upper half lane0)", r[4]);
    TEST_ASSERT(r[5] == 0.0f,  "vdpps 0xF1 r[5]=%f (upper half masked)", r[5]);

    /* mem operand form */
    __asm__ volatile(
        "vmovups %1, %%ymm0\n\t"
        "vdpps $0xFF, %2, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]), "m"(b[0]) : "ymm0","ymm2"
    );
    TEST_ASSERT(r[0] == 70.0f, "vdpps mem r[0]=%f", r[0]);
}

static void test_vdp_lane_boundaries_exceptions_and_vex_clear(void) {
    float a[8] = {1,2,3,4, 10,20,30,40};
    float b[8] = {5,6,7,8, 1,2,3,4};
    float r[8];
    __asm__ volatile(
        "vmovups %1,%%ymm0\n\tvmovups %2,%%ymm1\n\t"
        "vdpps $0xff,%%ymm1,%%ymm0,%%ymm2\n\tvmovups %%ymm2,%0"
        : "=m"(r) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2");
    for (int lane = 0; lane < 4; lane++)
        TEST_ASSERT(r[lane] == 70.0f, "vdpps distinct lower 128-bit group lane %d", lane);
    for (int lane = 4; lane < 8; lane++)
        TEST_ASSERT(r[lane] == 300.0f, "vdpps distinct upper 128-bit group lane %d", lane);

    __asm__ volatile(
        "vmovups %1,%%ymm0\n\t"
        "vdpps $0x81,%2,%%ymm0,%%ymm2\n\tvmovups %%ymm2,%0"
        : "=m"(r) : "m"(a), "m"(b) : "ymm0", "ymm2");
    for (int lane = 0; lane < 8; lane++) {
        float expected = lane == 0 ? 32.0f : lane == 4 ? 160.0f : 0.0f;
        TEST_ASSERT(r[lane] == expected,
                    "vdpps highest input selector and lowest output lane %d", lane);
    }

    ymm_t initial, result;
    xmm_t da = { .f64 = {2.0, 3.0} };
    xmm_t db = { .f64 = {4.0, 5.0} };
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile(
        "vmovdqu %1,%%ymm2\n\tvmovupd %2,%%xmm0\n\t"
        "vdppd $0x33,%3,%%xmm0,%%xmm2\n\tvmovdqu %%ymm2,%0"
        : "=m"(result) : "m"(initial), "m"(da), "m"(db)
        : "xmm0", "ymm2");
    TEST_ASSERT(result.f64[0] == 23.0 && result.f64[1] == 23.0,
                "vdppd VEX.128 low results");
    TEST_ASSERT(result.u64[2] == 0 && result.u64[3] == 0,
                "vdppd VEX.128 clears upper YMM");

    ymm_t exceptional_a = {0}, exceptional_b = {0};
    for (int lane = 0; lane < 8; lane++) {
        exceptional_a.f32[lane] = (float)(lane + 1);
        exceptional_b.f32[lane] = 1.0f;
    }
    exceptional_a.u32[3] = UINT32_C(0x7f812345);
    exceptional_a.u32[7] = UINT32_C(0x7f812345);
    uint32_t saved, csr;
    __asm__ volatile("stmxcsr %0" : "=m"(saved));
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile(
        "vmovups %0,%%ymm0\n\tvdpps $0x71,%1,%%ymm0,%%ymm2"
        : : "m"(exceptional_a), "m"(exceptional_b) : "ymm0", "ymm2");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(!(csr & 1u), "vdpps masked-off SNaN input does not set invalid");
    csr = saved & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile(
        "vmovups %0,%%ymm0\n\tvdpps $0xf1,%1,%%ymm0,%%ymm2"
        : : "m"(exceptional_a), "m"(exceptional_b) : "ymm0", "ymm2");
    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    TEST_ASSERT(csr & 1u, "vdpps selected SNaN input sets invalid");
    __asm__ volatile("ldmxcsr %0" : : "m"(saved));
}

static void test_vdp_immediate_boundaries(void) {
    xmm_t da = { .f64 = {2.0, 3.0} };
    xmm_t db = { .f64 = {4.0, 5.0} };
    xmm_t lane1_only, high_output, no_input, no_output, reserved_bits;
    __asm__ volatile (
        "vmovupd %5, %%xmm0\n\tvmovupd %6, %%xmm1\n\t"
        "vdppd $0x21, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %0\n\t"
        "vdppd $0x32, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %1\n\t"
        "vdppd $0x03, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %2\n\t"
        "vdppd $0x30, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %3\n\t"
        "vdppd $0xff, %%xmm1, %%xmm0, %%xmm2\n\t"
        "vmovupd %%xmm2, %4"
        : "=m"(lane1_only), "=m"(high_output), "=m"(no_input),
          "=m"(no_output), "=m"(reserved_bits)
        : "m"(da), "m"(db)
        : "xmm0", "xmm1", "xmm2");
    TEST_ASSERT(lane1_only.f64[0] == 15.0 && lane1_only.u64[1] == 0,
                "vdppd lane1 input selector and low output boundary");
    TEST_ASSERT(high_output.u64[0] == 0 && high_output.f64[1] == 23.0,
                "vdppd highest output selector");
    TEST_ASSERT(no_input.u64[0] == 0 && no_input.u64[1] == 0,
                "vdppd zero input mask produces exact +0 outputs");
    TEST_ASSERT(no_output.u64[0] == 0 && no_output.u64[1] == 0,
                "vdppd zero output mask clears all lanes");
    TEST_ASSERT(reserved_bits.f64[0] == 23.0 && reserved_bits.f64[1] == 23.0,
                "vdppd ignores reserved immediate bits in 0xff");

    ymm_t fa = { .f32 = {2, 20, 30, 40, 3, 50, 60, 70} };
    ymm_t fb = { .f32 = {4, 2, 3, 4, 5, 5, 6, 7} };
    ymm_t endpoint, ps_no_input, ps_no_output;
    __asm__ volatile (
        "vmovups %3, %%ymm0\n\tvmovups %4, %%ymm1\n\t"
        "vdpps $0x18, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %0\n\t"
        "vdpps $0x0f, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %1\n\t"
        "vdpps $0xf0, %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovups %%ymm2, %2"
        : "=m"(endpoint), "=m"(ps_no_input), "=m"(ps_no_output)
        : "m"(fa), "m"(fb)
        : "ymm0", "ymm1", "ymm2");
    for (int lane = 0; lane < 8; lane++) {
        uint32_t expected = lane == 3 ? UINT32_C(0x41000000) :
                            lane == 7 ? UINT32_C(0x41700000) : 0;
        TEST_ASSERT(endpoint.u32[lane] == expected,
                    "vdpps lowest input to highest output lane %d", lane);
        TEST_ASSERT(ps_no_input.u32[lane] == 0,
                    "vdpps zero input mask exact +0 lane %d", lane);
        TEST_ASSERT(ps_no_output.u32[lane] == 0,
                    "vdpps zero output mask exact +0 lane %d", lane);
    }
}

int main(void) {
    if (!check_avx()) { printf("AVX not supported\n"); return 1; }
    test_vdppd();
    test_vdppps();
    test_vdp_lane_boundaries_exceptions_and_vex_clear();
    test_vdp_immediate_boundaries();
    TEST_END();
}
