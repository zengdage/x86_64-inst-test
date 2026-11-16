#ifndef X86_64_AVX_COMI_TEST_H
#define X86_64_AVX_COMI_TEST_H

#include "../common.h"

#if TEST_BITS == 32
typedef float comi_t;
#define COMI_QNAN UINT32_C(0x7fc12345)
#define COMI_SNAN UINT32_C(0x7f812345)
#else
typedef double comi_t;
#define COMI_QNAN UINT64_C(0x7ff8123456789abc)
#define COMI_SNAN UINT64_C(0x7ff0123456789abc)
#endif

static void run_compare(comi_t a, comi_t b, uint8_t *cf, uint8_t *zf,
                        uint8_t *pf) {
    __asm__ volatile("vmovdqu %3, %%xmm0\n\tvmovdqu %4, %%xmm1\n\t"
                     TEST_INSN " %%xmm1, %%xmm0\n\t"
                     "setc %0\n\tsetz %1\n\tsetp %2"
                     : "=qm"(*cf), "=qm"(*zf), "=qm"(*pf)
                     : "m"(a), "m"(b) : "xmm0", "xmm1", "cc");
}

static void check_case(comi_t a, comi_t b, int cf_expected, int zf_expected,
                       int pf_expected, const char *name) {
    uint8_t cf, zf, pf;
    run_compare(a, b, &cf, &zf, &pf);
    TEST_ASSERT(cf == cf_expected && zf == zf_expected && pf == pf_expected,
                TEST_NAME " %s flags CF=%u ZF=%u PF=%u", name, cf, zf, pf);
}

int main(void) {
    union { comi_t value; __typeof__(COMI_QNAN) bits; } qnan = { .bits = COMI_QNAN };
    uint32_t original, cleared, observed;
    TEST_START(TEST_NAME);
    check_case((comi_t)1, (comi_t)2, 1, 0, 0, "less");
    check_case((comi_t)3, (comi_t)2, 0, 0, 0, "greater");
    check_case((comi_t)-0.0, (comi_t)0.0, 0, 1, 0, "signed zero equal");
    check_case(qnan.value, (comi_t)1, 1, 1, 1, "unordered QNaN");

    __asm__ volatile("stmxcsr %0" : "=m"(original));
    cleared = original & ~UINT32_C(0x3f);
    __asm__ volatile("ldmxcsr %0" : : "m"(cleared));
    run_compare(qnan.value, (comi_t)1, &(uint8_t){0}, &(uint8_t){0}, &(uint8_t){0});
    __asm__ volatile("stmxcsr %0\n\tldmxcsr %1"
                     : "=m"(observed) : "m"(original) : "memory");
#if TEST_ORDERED
    TEST_ASSERT(observed & 1U, TEST_NAME " QNaN sets MXCSR invalid");
#else
    TEST_ASSERT(!(observed & 1U), TEST_NAME " QNaN does not set MXCSR invalid");
#endif
    TEST_END();
}

#endif
