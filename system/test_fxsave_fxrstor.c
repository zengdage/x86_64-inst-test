#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int has_fxsave(void) {
    uint32_t eax = 1, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
    return (edx & (1u << 24)) != 0;
}
#else
#define has_fxsave() 1
#endif

typedef struct {
    uint8_t bytes[512];
} __attribute__((aligned(16))) fx_area_t;

typedef struct {
    uint8_t bytes[10];
} ext80_t;

static void test_fxsave_roundtrip(void) {
    fx_area_t original, state;
    const xmm_t xmm_before = {
        .u64 = {UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210)}};
    xmm_t xmm_after;
    const ext80_t x87_before = {{0x35, 0xc2, 0x68, 0x21, 0xa2, 0xda, 0x0f, 0xc9, 0x00, 0x40}};
    ext80_t x87_after;
    uint32_t mxcsr_before, mxcsr_after;
    __asm__ volatile ("stmxcsr %0" : "=m"(mxcsr_before));
    uint32_t test_mxcsr = (mxcsr_before & ~UINT32_C(0x6000)) | UINT32_C(0x4000);
    memset(&original, 0, sizeof(original));
    memset(&state, 0, sizeof(state));

    __asm__ volatile (
        "fxsave %[original]\n\t"
        "fninit\n\tfldt %[x87_before]\n\tldmxcsr %[test_mxcsr]\n\t"
        "movdqu %[xmm_before], %%xmm0\n\tfxsave %[state]\n\t"
        "fninit\n\tpxor %%xmm0, %%xmm0\n\tldmxcsr %[mxcsr_before]\n\t"
        "fxrstor %[state]\n\tmovdqu %%xmm0, %[xmm_after]\n\t"
        "fstpt %[x87_after]\n\tstmxcsr %[mxcsr_after]\n\t"
        "fxrstor %[original]"
        : [original] "+m"(original), [state] "+m"(state),
          [xmm_after] "=m"(xmm_after), [x87_after] "=m"(x87_after),
          [mxcsr_after] "=m"(mxcsr_after)
        : [xmm_before] "m"(xmm_before), [x87_before] "m"(x87_before),
          [test_mxcsr] "m"(test_mxcsr), [mxcsr_before] "m"(mxcsr_before)
        : "xmm0", "memory");

    uint16_t saved_cw, saved_sw;
    uint32_t saved_mxcsr;
    memcpy(&saved_cw, state.bytes, sizeof(saved_cw));
    memcpy(&saved_sw, state.bytes + 2, sizeof(saved_sw));
    memcpy(&saved_mxcsr, state.bytes + 24, sizeof(saved_mxcsr));
    TEST_ASSERT(saved_cw == UINT16_C(0x037f), "fxsave stores x87 control word");
    TEST_ASSERT((saved_sw & UINT16_C(0x3800)) == UINT16_C(0x3800),
                "fxsave stores one pushed value with x87 TOP=7");
    TEST_ASSERT(saved_mxcsr == test_mxcsr, "fxsave stores MXCSR: %#x", saved_mxcsr);
    TEST_ASSERT(memcmp(&xmm_before, &xmm_after, sizeof(xmm_before)) == 0,
                "fxrstor restores XMM0 exactly");
    TEST_ASSERT(memcmp(&x87_before, &x87_after, sizeof(x87_before)) == 0,
                "fxrstor restores exact 80-bit ST(0)");
    TEST_ASSERT(mxcsr_after == test_mxcsr, "fxrstor restores MXCSR");
}

static void test_64bit_forms(void) {
    fx_area_t original, state;
    const xmm_t xmm_before = {
        .u64 = {UINT64_C(0xa5a55a5af0f00f0f), UINT64_C(0x0123456789abcdef)}};
    xmm_t xmm_after;
    const ext80_t x87_before = {{0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x81, 0x00, 0x40}};
    ext80_t x87_after;
    uint32_t mxcsr_before, mxcsr_after;
    const unsigned char *x87_ip;
    __asm__ volatile ("stmxcsr %0" : "=m"(mxcsr_before));
    uint32_t test_mxcsr = (mxcsr_before & ~UINT32_C(0x6000)) | UINT32_C(0x2000);
    memset(&original, 0, sizeof(original));
    memset(&state, 0, sizeof(state));
    __asm__ volatile (
        "fxsave64 %[original]\n\t"
        "fninit\n\t0: fldt %[x87_before]\n\t"
        "leaq 0b(%%rip), %[x87_ip]\n\t"
        "ldmxcsr %[test_mxcsr]\n\tmovdqu %[xmm_before], %%xmm1\n\t"
        "fxsave64 %[state]\n\t"
        "fninit\n\tpxor %%xmm1, %%xmm1\n\tldmxcsr %[mxcsr_before]\n\t"
        "fxrstor64 %[state]\n\tmovdqu %%xmm1, %[xmm_after]\n\t"
        "fstpt %[x87_after]\n\tstmxcsr %[mxcsr_after]\n\t"
        "fxrstor64 %[original]"
        : [original] "+m"(original), [state] "+m"(state),
          [xmm_after] "=m"(xmm_after), [x87_after] "=m"(x87_after),
          [mxcsr_after] "=m"(mxcsr_after), [x87_ip] "=&r"(x87_ip)
        : [xmm_before] "m"(xmm_before), [x87_before] "m"(x87_before),
          [test_mxcsr] "m"(test_mxcsr), [mxcsr_before] "m"(mxcsr_before)
        : "xmm1", "memory");

    uint16_t saved_cw, saved_sw;
    uint32_t saved_mxcsr;
    uint64_t saved_fip;
    memcpy(&saved_cw, state.bytes, sizeof(saved_cw));
    memcpy(&saved_sw, state.bytes + 2, sizeof(saved_sw));
    memcpy(&saved_fip, state.bytes + 8, sizeof(saved_fip));
    memcpy(&saved_mxcsr, state.bytes + 24, sizeof(saved_mxcsr));
    TEST_ASSERT(saved_cw == UINT16_C(0x037f), "fxsave64 stores x87 control word");
    TEST_ASSERT((saved_sw & UINT16_C(0x3800)) == UINT16_C(0x3800),
                "fxsave64 stores x87 TOP=7");
    TEST_ASSERT(saved_fip == (uintptr_t)x87_ip,
                "fxsave64 stores full 64-bit x87 FIP: %#" PRIx64, saved_fip);
    TEST_ASSERT(saved_mxcsr == test_mxcsr, "fxsave64 stores MXCSR");
    TEST_ASSERT(memcmp(&xmm_before, &xmm_after, sizeof(xmm_before)) == 0,
                "fxrstor64 restores XMM1 exactly");
    TEST_ASSERT(memcmp(&x87_before, &x87_after, sizeof(x87_before)) == 0,
                "fxrstor64 restores exact 80-bit ST(0)");
    TEST_ASSERT(mxcsr_after == test_mxcsr, "fxrstor64 restores MXCSR");
}

int main(void) {
    TEST_START("FXSAVE/FXRSTOR state");
    if (!has_fxsave()) {
        printf("  SKIP: CPU lacks FXSAVE/FXRSTOR\n");
        TEST_END();
    }
    test_fxsave_roundtrip();
    test_64bit_forms();
    TEST_END();
}
