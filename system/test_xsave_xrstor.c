/*
 * test_xsave_xrstor.c - Test XSAVE and XRSTOR instructions
 *
 * XSAVE saves a set of processor state components to a memory region.
 * XRSTOR restores processor state components from a memory region.
 * The state components to save/restore are specified by EDX:EAX mask,
 * intersected with XCR0.
 *
 * Prerequisites:
 *   - CPUID.01H:ECX[26] (XSAVE bit) must be set
 *   - CPUID.01H:ECX[27] (OSXSAVE bit) must be set (OS has enabled XSAVE)
 *
 * Compile: gcc -o test_xsave_xrstor system/test_xsave_xrstor.c -O0
 * Note: Do not use static linking (-static)
 */

#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int has_osxsave(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    return (ecx >> 27) & 1;
}
#else
#define has_osxsave() 1
#endif

static uint64_t get_xcr0(void)
{
    uint32_t lo, hi;
    __asm__ volatile (
        "xgetbv"
        : "=a"(lo), "=d"(hi)
        : "c"(0)
    );
    return ((uint64_t)hi << 32) | lo;
}

/* Get the required size for XSAVE area via CPUID leaf 0x0D */
static uint32_t get_xsave_size(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x0D), "c"(0)
    );
    /* ECX = size required for all enabled components */
    return ecx;
}

/* Test CPUID leaf 0x0D: XSAVE area info */
static void test_xsave_cpuid_info(void)
{
    uint32_t eax, ebx, ecx, edx;

    TEST_START("XSAVE - CPUID leaf 0x0D information");

    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x0D), "c"(0)
    );

    printf("  Subleaf 0: EAX=0x%08X (supported low bits of XCR0)\n", eax);
    printf("  Subleaf 0: EBX=%u (max size for currently enabled features)\n", ebx);
    printf("  Subleaf 0: ECX=%u (max size for all supported features)\n", ecx);
    printf("  Subleaf 0: EDX=0x%08X (supported high bits of XCR0)\n", edx);

    TEST_ASSERT(ecx >= 512, "XSAVE area must be at least 512 bytes (FXSAVE legacy), got %u", ecx);
    TEST_ASSERT(ebx > 0, "Current XSAVE size should be > 0, got %u", ebx);

    /* Subleaf 1: XSAVE features */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x0D), "c"(1)
    );

    printf("  Subleaf 1: EAX=0x%08X\n", eax);
    printf("    XSAVEOPT: %s\n", (eax & (1U << 0)) ? "yes" : "no");
    printf("    XSAVEC:   %s\n", (eax & (1U << 1)) ? "yes" : "no");
    printf("    XGETBV ECX=1: %s\n", (eax & (1U << 2)) ? "yes" : "no");
    printf("    XSAVES:   %s\n", (eax & (1U << 3)) ? "yes" : "no");
}

/* Test XSAVE/XRSTOR with x87 and SSE state (bits 0 and 1) */
static void test_xsave_xrstor_basic(void)
{
    uint64_t xcr0 = get_xcr0();
    uint32_t xsave_size = get_xsave_size();

    TEST_START("XSAVE/XRSTOR - Save and restore x87+SSE state");

    printf("  XCR0: 0x%016" PRIX64 "\n", xcr0);
    printf("  XSAVE area size: %u bytes\n", xsave_size);

    /* Allocate aligned buffer for XSAVE area */
    uint8_t *xsave_area = NULL;
    if (posix_memalign((void **)&xsave_area, 64, xsave_size) != 0) {
        TEST_ASSERT(0, "Failed to allocate aligned XSAVE area");
        return;
    }
    memset(xsave_area, 0, xsave_size);

    /* Save x87+SSE state (mask = 0x3: bits 0 and 1) */
    uint32_t mask_lo = 0x3;  /* x87 + SSE */
    uint32_t mask_hi = 0x0;

    __asm__ volatile (
        "xsave %0"
        : "=m"(*xsave_area)
        : "a"(mask_lo), "d"(mask_hi)
        : "memory"
    );

    /* Check XSAVE header: bytes 512-519 contain XSTATE_BV */
    uint64_t xstate_bv;
    memcpy(&xstate_bv, xsave_area + 512, sizeof(xstate_bv));
    printf("  XSTATE_BV after XSAVE: 0x%016" PRIX64 "\n", xstate_bv);

    TEST_ASSERT((xstate_bv & ~UINT64_C(0x3)) == 0,
                "XSAVE x87+SSE request must not set unrelated XSTATE_BV bits: 0x%" PRIX64,
                xstate_bv);

    /* Now restore the state */
    __asm__ volatile (
        "xrstor %0"
        :
        : "m"(*xsave_area), "a"(mask_lo), "d"(mask_hi)
        : "memory"
    );

    free(xsave_area);
}

/* Test XSAVE/XRSTOR preserves SSE register values */
static void test_xsave_xrstor_sse_roundtrip(void)
{
    uint64_t xcr0 = get_xcr0();
    uint32_t xsave_size = get_xsave_size();

    TEST_START("XSAVE/XRSTOR - SSE register round-trip");

    if (!(xcr0 & 0x2)) {
        printf("  SSE not enabled in XCR0, skipping\n");
        return;
    }

    uint8_t *xsave_area = NULL;
    if (posix_memalign((void **)&xsave_area, 64, xsave_size) != 0) {
        TEST_ASSERT(0, "Failed to allocate aligned XSAVE area");
        return;
    }
    memset(xsave_area, 0, xsave_size);

    /* Set XMM0 to a known pattern */
    xmm_t before __attribute__((aligned(16)));
    xmm_t after __attribute__((aligned(16)));

    before.u64[0] = 0xDEADBEEFCAFEBABEULL;
    before.u64[1] = 0x0123456789ABCDEFULL;

    printf("  Setting XMM0 to: 0x%016" PRIX64 "%016" PRIX64 "\n", before.u64[1], before.u64[0]);

    /* XSAVE all enabled state */
    uint32_t mask_lo = (uint32_t)(xcr0 & 0xFFFFFFFF);
    uint32_t mask_hi = (uint32_t)(xcr0 >> 32);

    /*
     * Set XMM0 and XSAVE in one asm block to prevent the compiler or
     * any intervening function call (like printf) from clobbering XMM0.
     * We load the pattern address into a register and use movdqa from (reg).
     */
    __asm__ volatile (
        "movdqa (%3), %%xmm0\n\t"
        "xsave (%4)"
        : "=m"(*xsave_area)
        : "a"(mask_lo), "d"(mask_hi), "r"(&before), "r"(xsave_area)
        : "xmm0", "memory"
    );

    /* Clobber XMM0 */
    __asm__ volatile (
        "pxor %%xmm0, %%xmm0"
        ::: "xmm0"
    );

    /* Verify it was clobbered */
    __asm__ volatile (
        "movdqa %%xmm0, %0"
        : "=m"(after)
        :
        : "memory"
    );
    printf("  XMM0 after clobber: 0x%016" PRIX64 "%016" PRIX64 "\n", after.u64[1], after.u64[0]);
    TEST_ASSERT(after.u64[0] == 0 && after.u64[1] == 0, "XMM0 should be zero after PXOR");

    /* XRSTOR to recover state */
    __asm__ volatile (
        "xrstor %0"
        :
        : "m"(*xsave_area), "a"(mask_lo), "d"(mask_hi)
        : "memory"
    );

    /* Read XMM0 back */
    __asm__ volatile (
        "movdqa %%xmm0, %0"
        : "=m"(after)
        :
        : "memory"
    );

    printf("  XMM0 after XRSTOR: 0x%016" PRIX64 "%016" PRIX64 "\n", after.u64[1], after.u64[0]);

    TEST_ASSERT(after.u64[0] == before.u64[0] && after.u64[1] == before.u64[1],
                "XMM0 should be restored: expected 0x%016" PRIX64 "%016" PRIX64
                ", got 0x%016" PRIX64 "%016" PRIX64,
                before.u64[1], before.u64[0], after.u64[1], after.u64[0]);

    free(xsave_area);
}

/* Test XSAVE area structure: legacy region (first 512 bytes) */
static void test_xsave_legacy_region(void)
{
    uint64_t xcr0 = get_xcr0();
    uint32_t xsave_size = get_xsave_size();

    TEST_START("XSAVE - Legacy region inspection");

    uint8_t *xsave_area = NULL;
    if (posix_memalign((void **)&xsave_area, 64, xsave_size) != 0) {
        TEST_ASSERT(0, "Failed to allocate aligned XSAVE area");
        return;
    }
    memset(xsave_area, 0, xsave_size);

    uint32_t mask_lo = (uint32_t)(xcr0 & 0xFFFFFFFF);
    uint32_t mask_hi = (uint32_t)(xcr0 >> 32);

    __asm__ volatile (
        "xsave %0"
        : "=m"(*xsave_area)
        : "a"(mask_lo), "d"(mask_hi)
        : "memory"
    );

    /* Legacy region: bytes 0-511 match FXSAVE format */
    uint16_t fcw;
    memcpy(&fcw, xsave_area + 0, sizeof(fcw));
    printf("  FCW (FPU Control Word): 0x%04X\n", fcw);

    uint16_t fsw;
    memcpy(&fsw, xsave_area + 2, sizeof(fsw));
    printf("  FSW (FPU Status Word): 0x%04X\n", fsw);

    uint32_t mxcsr;
    memcpy(&mxcsr, xsave_area + 24, sizeof(mxcsr));
    printf("  MXCSR: 0x%08X\n", mxcsr);

    uint32_t mxcsr_mask;
    memcpy(&mxcsr_mask, xsave_area + 28, sizeof(mxcsr_mask));
    printf("  MXCSR_MASK: 0x%08X\n", mxcsr_mask);

    /* XSAVE header at offset 512 */
    uint64_t xstate_bv, xcomp_bv;
    memcpy(&xstate_bv, xsave_area + 512, sizeof(xstate_bv));
    memcpy(&xcomp_bv, xsave_area + 520, sizeof(xcomp_bv));
    printf("  XSTATE_BV: 0x%016" PRIX64 "\n", xstate_bv);
    printf("  XCOMP_BV:  0x%016" PRIX64 "\n", xcomp_bv);

    /* Default FCW on x86-64 is typically 0x037F */
    TEST_ASSERT(fcw != 0, "FCW should not be zero after XSAVE");
    /* MXCSR default is 0x1F80 */
    TEST_ASSERT(mxcsr != 0, "MXCSR should not be zero after XSAVE");

    free(xsave_area);
}

/* Test XSAVE with zero mask: should be a no-op that doesn't write state */
static void test_xsave_zero_mask(void)
{
    uint32_t xsave_size = get_xsave_size();

    TEST_START("XSAVE - Zero mask (no state saved)");

    uint8_t *xsave_area = NULL;
    if (posix_memalign((void **)&xsave_area, 64, xsave_size) != 0) {
        TEST_ASSERT(0, "Failed to allocate aligned XSAVE area");
        return;
    }

    /* Fill with a marker pattern */
    memset(xsave_area, 0xAA, xsave_size);
    /* But zero out the XSAVE header as required for XRSTOR compatibility */
    memset(xsave_area + 512, 0, 64);

    /* XSAVE with mask=0: should not write any state components */
    __asm__ volatile (
        "xsave %0"
        : "=m"(*xsave_area)
        : "a"((uint32_t)0), "d"((uint32_t)0)
        : "memory"
    );

    /* XSTATE_BV should be 0 since no components were requested */
    uint64_t xstate_bv;
    memcpy(&xstate_bv, xsave_area + 512, sizeof(xstate_bv));
    printf("  XSTATE_BV with zero mask: 0x%016" PRIX64 "\n", xstate_bv);

    TEST_ASSERT(xstate_bv == 0,
                "XSTATE_BV should be 0 with zero mask, got 0x%" PRIX64, xstate_bv);

    free(xsave_area);
}

__attribute__((target("avx,avx512f")))
static void test_xsave_extended_roundtrips(void)
{
    uint64_t xcr0 = get_xcr0();
    uint32_t size = get_xsave_size();
    uint8_t *area = NULL;
    TEST_START("XSAVE/XRSTOR - x87, YMM, opmask/ZMM and PKRU round-trips");
    if (posix_memalign((void **)&area, 64, size) != 0) {
        TEST_ASSERT(0, "extended XSAVE allocation");
        return;
    }

    memset(area, 0, size);
    long double x87_before = 0x9.abcdef012345678p-3L, x87_after = 0.0L;
    uint32_t lo = 1, hi = 0;
    __asm__ volatile("fldt %0\n\txsave (%1)" : : "m"(x87_before), "r"(area), "a"(lo), "d"(hi) : "memory");
    __asm__ volatile("fstp %%st(0)\n\txrstor (%1)\n\tfstpt %0" : "=m"(x87_after) : "r"(area), "a"(lo), "d"(hi) : "memory");
    TEST_ASSERT(x87_after == x87_before, "XSAVE x87 ST0 round-trip");

    if (xcr0 & (1u << 2)) {
        ymm_t before, after;
        for (int i = 0; i < 4; i++) before.u64[i] = UINT64_C(0x1111111111111111) * (uint64_t)(i + 1);
        memset(&after, 0, sizeof(after)); memset(area, 0, size);
        lo = 0x7;
        __asm__ volatile("vmovdqu (%0),%%ymm0\n\txsave (%1)" : : "r"(&before), "r"(area), "a"(lo), "d"(hi) : "ymm0", "memory");
        __asm__ volatile("vpxor %%ymm0,%%ymm0,%%ymm0\n\txrstor (%1)\n\tvmovdqu %%ymm0,(%0)"
            : : "r"(&after), "r"(area), "a"(lo), "d"(hi) : "ymm0", "memory");
        TEST_ASSERT(memcmp(&before, &after, sizeof(before)) == 0, "XSAVE YMM0 including upper 128 bits round-trip");
        uint64_t xstate_bv; memcpy(&xstate_bv, area + 512, sizeof(xstate_bv));
        TEST_ASSERT(xstate_bv & (1u << 2), "XSAVE YMM state sets XSTATE_BV bit 2");
    }

    if ((xcr0 & 0xe0) == 0xe0) {
        uint8_t before[64] __attribute__((aligned(64)));
        uint8_t after[64] __attribute__((aligned(64)));
        uint64_t k_before = 0xa5, k_after = 0;
        for (int i = 0; i < 64; i++) before[i] = (uint8_t)(i * 3 + 1);
        memset(after, 0, sizeof(after)); memset(area, 0, size);
        lo = (uint32_t)(xcr0 & 0xff);
        __asm__ volatile("vmovdqu64 (%0),%%zmm16\n\tkmovq %2,%%k1\n\txsave (%1)"
            : : "r"(before), "r"(area), "r"(k_before), "a"(lo), "d"(hi) : "zmm16", "k1", "memory");
        __asm__ volatile("vpxord %%zmm16,%%zmm16,%%zmm16\n\tkxorw %%k1,%%k1,%%k1\n\txrstor (%2)\n\tvmovdqu64 %%zmm16,%0\n\tkmovq %%k1,%1"
            : "=m"(*(uint8_t (*)[64])after), "=r"(k_after) : "r"(area), "a"(lo), "d"(hi) : "zmm16", "k1", "memory");
        TEST_ASSERT(memcmp(before, after, sizeof(before)) == 0, "XSAVE ZMM16 round-trip");
        TEST_ASSERT((k_after & 0xff) == k_before, "XSAVE opmask k1 round-trip");
    }

    if (xcr0 & (1u << 9)) {
        uint32_t pkru_before, pkru_after;
        memset(area, 0, size);
        __asm__ volatile("xor %%ecx,%%ecx\n\trdpkru" : "=a"(pkru_before) : : "ecx", "edx");
        lo = 1u << 9;
        __asm__ volatile("xsave (%0)" : : "r"(area), "a"(lo), "d"(hi) : "memory");
        uint32_t alternate = pkru_before ^ (3u << 30);
        __asm__ volatile("xor %%ecx,%%ecx\n\txor %%edx,%%edx\n\twrpkru" : : "a"(alternate) : "ecx", "edx", "memory");
        __asm__ volatile("xrstor (%0)" : : "r"(area), "a"(lo), "d"(hi) : "memory");
        __asm__ volatile("xor %%ecx,%%ecx\n\trdpkru" : "=a"(pkru_after) : : "ecx", "edx");
        TEST_ASSERT(pkru_after == pkru_before, "XSAVE PKRU round-trip");
    }

    free(area);
}

int main(void)
{
    printf("Checking XSAVE/OSXSAVE support...\n");

    if (!has_osxsave()) {
        printf("OSXSAVE not set (CPUID.01H:ECX[27] = 0). XSAVE not enabled by OS. Skipping.\n");
        TEST_ASSERT(1, "XSAVE not enabled - skip is expected");
        TEST_END();
    }

    printf("  OSXSAVE is enabled\n\n");

    test_xsave_cpuid_info();
    test_xsave_xrstor_basic();
    test_xsave_xrstor_sse_roundtrip();
    test_xsave_legacy_region();
    test_xsave_zero_mask();
    test_xsave_extended_roundtrips();

    TEST_END();
}
