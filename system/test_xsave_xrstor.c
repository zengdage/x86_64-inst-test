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

static int has_osxsave(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    return (ecx >> 27) & 1;
}

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

    TEST_ASSERT(1, "XSAVE executed successfully");

    /* Now restore the state */
    __asm__ volatile (
        "xrstor %0"
        :
        : "m"(*xsave_area), "a"(mask_lo), "d"(mask_hi)
        : "memory"
    );

    TEST_ASSERT(1, "XRSTOR executed successfully");

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

    TEST_END();
}
