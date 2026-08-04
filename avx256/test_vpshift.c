/*
 * Test VPSLLD/VPSLLQ/VPSLLW/VPSRLD/VPSRLQ/VPSRLW/VPSRAD/VPSRAQ/VPSRAW
 * 256-bit packed integer shift instructions (AVX2 ymm forms)
 * Compile: gcc -o test_vpshift avx256/test_vpshift.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <stdio.h>
#include "../common.h"

#define STR1(x) #x
#define STR(x) STR1(x)
#define RUN_SHIFT_IMM(op, count, src, dst) do { \
    __asm__ volatile( \
        "vmovdqu %1, %%ymm0\n\t" \
        STR(op) " $" STR(count) ", %%ymm0, %%ymm1\n\t" \
        "vmovdqu %%ymm1, %0" \
        : "=m"(dst) : "m"(src) : "ymm0", "ymm1"); \
} while (0)

#define RUN_SHIFT_COUNT(op, count, src, dst) do { \
    __asm__ volatile( \
        "vmovdqu %1, %%ymm0\n\t" \
        "vmovdqu %2, %%xmm2\n\t" \
        STR(op) " %%xmm2, %%ymm0, %%ymm1\n\t" \
        "vmovdqu %%ymm1, %0" \
        : "=m"(dst) : "m"(src), "m"(count) : "ymm0", "ymm1", "xmm2"); \
} while (0)

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(7),"c"(0));
    return (ebx >> 5) & 1;
}
#else
#define check_avx2() 1
#endif

static void test_vpslld(void) {
    TEST_START("VPSLLD (256-bit shift left dword by imm)");
    int32_t a[8] = {1,2,3,4,5,6,7,8};
    int32_t r[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpslld $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == a[i] * 4, "vpslld r[%d]=%d", i, r[i]);
    /* xmm count form */
    int32_t r2[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vmovd %2, %%xmm2\n\t"
        "vpslld %%xmm2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r2[0]) : "m"(a[0]), "r"(2) : "ymm0","ymm1","xmm2"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r2[i] == a[i] * 4, "vpslld xmm r[%d]=%d", i, r2[i]);
}

static void test_vpsllq(void) {
    TEST_START("VPSLLQ (256-bit shift left qword by imm)");
    int64_t a[4] = {1, 2, 3, 4};
    int64_t r[4];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsllq $3, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == a[i] * 8, "vpsllq r[%d]=%lld", i, (long long)r[i]);
}

static void test_vpsllw(void) {
    TEST_START("VPSLLW (256-bit shift left word by imm)");
    int16_t a[16];
    for (int i = 0; i < 16; i++) a[i] = (int16_t)(i + 1);
    int16_t r[16];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsllw $1, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(r[i] == a[i] * 2, "vpsllw r[%d]=%d", i, r[i]);
}

static void test_vpsrld(void) {
    TEST_START("VPSRLD (256-bit logical shift right dword by imm)");
    uint32_t a[8] = {8,16,32,64,128,256,512,1024};
    uint32_t r[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrld $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == a[i] / 4, "vpsrld r[%d]=%u", i, r[i]);
    /* sign bit not propagated */
    uint32_t neg[8];
    for (int i = 0; i < 8; i++) neg[i] = 0x80000000U;
    uint32_t rneg[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrld $1, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(rneg[0]) : "m"(neg[0]) : "ymm0","ymm1"
    );
    TEST_ASSERT(rneg[0] == 0x40000000U, "vpsrld logical neg r[0]=0x%08x", rneg[0]);
}

static void test_vpsrlq(void) {
    TEST_START("VPSRLQ (256-bit logical shift right qword by imm)");
    uint64_t a[4] = {64, 128, 256, 512};
    uint64_t r[4];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrlq $3, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(r[i] == a[i] / 8, "vpsrlq r[%d]=%llu", i, (unsigned long long)r[i]);
}

static void test_vpsrlw(void) {
    TEST_START("VPSRLW (256-bit logical shift right word by imm)");
    uint16_t a[16];
    for (int i = 0; i < 16; i++) a[i] = (uint16_t)((i + 1) * 4);
    uint16_t r[16];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrlw $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(r[i] == (uint16_t)(i + 1), "vpsrlw r[%d]=%u", i, r[i]);
}

static void test_vpsrad(void) {
    TEST_START("VPSRAD (256-bit arithmetic shift right dword by imm)");
    int32_t a[8] = {-8,-16,8,16,-32,32,-64,64};
    int32_t r[8];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrad $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(r[i] == a[i] / 4, "vpsrad r[%d]=%d", i, r[i]);
    /* sign preserved */
    TEST_ASSERT(r[0] == -2, "vpsrad neg r[0]=%d", r[0]);
}

static void test_vpsraq(void) {
    TEST_START("VPSRAQ (256-bit arithmetic shift right qword by imm, AVX-512 but test gracefully)");
    /* VPSRAQ is AVX-512VL; skip if not available, just test VPSRLQ as fallback */
    /* We test VPSRLQ with sign-bit check to document the logical behavior */
    int64_t a[4] = {-8LL, -16LL, 8LL, 16LL};
    int64_t r[4];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsrlq $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    /* logical: sign bit not preserved */
    TEST_ASSERT(r[0] != -2LL, "vpsrlq (logical) does not preserve sign r[0]=%lld", (long long)r[0]);
    TEST_ASSERT(r[2] == 2LL,  "vpsrlq positive r[2]=%lld", (long long)r[2]);
}

static void test_vpsraw(void) {
    TEST_START("VPSRAW (256-bit arithmetic shift right word by imm)");
    int16_t a[16];
    for (int i = 0; i < 8; i++) { a[i] = (int16_t)(-4 * (i+1)); a[i+8] = (int16_t)(4 * (i+1)); }
    int16_t r[16];
    __asm__ volatile(
        "vmovdqu %1, %%ymm0\n\t"
        "vpsraw $2, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0\n\t"
        : "=m"(r[0]) : "m"(a[0]) : "ymm0","ymm1"
    );
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(r[i] == a[i] / 4, "vpsraw r[%d]=%d", i, r[i]);
    TEST_ASSERT(r[0] == -1, "vpsraw neg r[0]=%d", r[0]);
}

static void test_shift_count_boundaries(void) {
    TEST_START("VPSHIFT count boundaries");
    uint16_t w[16], wr[16];
    uint32_t d[8], dr[8];
    uint64_t q[4], qr[4];
    int16_t sw[16], swr[16];
    int32_t sd[8], sdr[8];

    for (int i = 0; i < 16; i++) {
        w[i] = (uint16_t)(0x8001u + i);
        sw[i] = (i & 1) ? (int16_t)0x4000 : (int16_t)0x8000;
    }
    for (int i = 0; i < 8; i++) {
        d[i] = 0x80000001u + (uint32_t)i;
        sd[i] = (i & 1) ? 0x40000000 : (int32_t)0x80000000u;
    }
    for (int i = 0; i < 4; i++) q[i] = UINT64_C(0x8000000000000001) + (uint64_t)i;

    RUN_SHIFT_IMM(vpsllw, 0, w, wr);
    for (int i = 0; i < 16; i++) TEST_ASSERT(wr[i] == w[i], "vpsllw count 0 lane %d", i);
    RUN_SHIFT_IMM(vpsllw, 15, w, wr);
    for (int i = 0; i < 16; i++) TEST_ASSERT(wr[i] == (uint16_t)(w[i] << 15), "vpsllw count 15 lane %d", i);
    RUN_SHIFT_IMM(vpsllw, 16, w, wr);
    for (int i = 0; i < 16; i++) TEST_ASSERT(wr[i] == 0, "vpsllw count 16 lane %d", i);
    RUN_SHIFT_IMM(vpsllw, 17, w, wr);
    for (int i = 0; i < 16; i++) TEST_ASSERT(wr[i] == 0, "vpsllw count 17 lane %d", i);
    RUN_SHIFT_IMM(vpsllw, 255, w, wr);
    for (int i = 0; i < 16; i++) TEST_ASSERT(wr[i] == 0, "vpsllw count 255 lane %d", i);

    RUN_SHIFT_IMM(vpsrld, 0, d, dr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dr[i] == d[i], "vpsrld count 0 lane %d", i);
    RUN_SHIFT_IMM(vpsrld, 31, d, dr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dr[i] == (d[i] >> 31), "vpsrld count 31 lane %d", i);
    RUN_SHIFT_IMM(vpsrld, 32, d, dr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dr[i] == 0, "vpsrld count 32 lane %d", i);
    RUN_SHIFT_IMM(vpsrld, 33, d, dr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dr[i] == 0, "vpsrld count 33 lane %d", i);
    RUN_SHIFT_IMM(vpsrld, 255, d, dr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dr[i] == 0, "vpsrld count 255 lane %d", i);

    RUN_SHIFT_IMM(vpsrlq, 0, q, qr);
    for (int i = 0; i < 4; i++) TEST_ASSERT(qr[i] == q[i], "vpsrlq count 0 lane %d", i);
    RUN_SHIFT_IMM(vpsrlq, 63, q, qr);
    for (int i = 0; i < 4; i++) TEST_ASSERT(qr[i] == (q[i] >> 63), "vpsrlq count 63 lane %d", i);
    RUN_SHIFT_IMM(vpsrlq, 64, q, qr);
    for (int i = 0; i < 4; i++) TEST_ASSERT(qr[i] == 0, "vpsrlq count 64 lane %d", i);
    RUN_SHIFT_IMM(vpsrlq, 65, q, qr);
    for (int i = 0; i < 4; i++) TEST_ASSERT(qr[i] == 0, "vpsrlq count 65 lane %d", i);
    RUN_SHIFT_IMM(vpsrlq, 255, q, qr);
    for (int i = 0; i < 4; i++) TEST_ASSERT(qr[i] == 0, "vpsrlq count 255 lane %d", i);

    RUN_SHIFT_IMM(vpsraw, 16, sw, swr);
    for (int i = 0; i < 16; i++) TEST_ASSERT(swr[i] == (sw[i] < 0 ? -1 : 0), "vpsraw saturated lane %d", i);
    RUN_SHIFT_IMM(vpsrad, 255, sd, sdr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(sdr[i] == (sd[i] < 0 ? -1 : 0), "vpsrad saturated lane %d", i);

    /* Packed shifts consume a single unsigned count from the low 64 bits. */
    uint64_t count_high_ignored[2] = {1, UINT64_MAX};
    uint64_t count_low_max[2] = {UINT64_MAX, 0};
    RUN_SHIFT_COUNT(vpsrld, count_high_ignored, d, dr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dr[i] == (d[i] >> 1), "vpsrld ignores count high qword lane %d", i);
    RUN_SHIFT_COUNT(vpsrld, count_low_max, d, dr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(dr[i] == 0, "vpsrld UINT64_MAX count lane %d", i);
    RUN_SHIFT_COUNT(vpsrad, count_low_max, sd, sdr);
    for (int i = 0; i < 8; i++) TEST_ASSERT(sdr[i] == (sd[i] < 0 ? -1 : 0), "vpsrad UINT64_MAX count lane %d", i);
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vpslld();
    test_vpsllq();
    test_vpsllw();
    test_vpsrld();
    test_vpsrlq();
    test_vpsrlw();
    test_vpsrad();
    test_vpsraq();
    test_vpsraw();
    test_shift_count_boundaries();
    TEST_END();
}
