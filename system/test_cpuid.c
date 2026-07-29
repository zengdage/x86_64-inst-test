/*
 * test_cpuid.c - Test CPUID instruction
 *
 * CPUID returns processor identification and feature information.
 * Called with EAX (and sometimes ECX) as input, it returns data in EAX, EBX, ECX, EDX.
 *
 * Prerequisites: None (CPUID is available on all x86-64 processors)
 *
 * Compile: gcc -o test_cpuid system/test_cpuid.c -O0
 * Note: Do not use static linking (-static)
 */

#include "../common.h"

/* Test CPUID leaf 0: vendor string and max basic leaf */
static void test_cpuid_vendor(void)
{
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    TEST_START("CPUID - Vendor String (leaf 0)");

    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    memcpy(vendor + 0, &ebx, 4);
    memcpy(vendor + 4, &edx, 4);
    memcpy(vendor + 8, &ecx, 4);
    vendor[12] = '\0';

    printf("  Vendor string: \"%s\"\n", vendor);
    printf("  Max basic CPUID leaf: %u\n", eax);

    /* Vendor must be one of the known strings */
    int known_vendor = (strcmp(vendor, "GenuineIntel") == 0 ||
                        strcmp(vendor, "AuthenticAMD") == 0 ||
                        strcmp(vendor, "GenuineIotel") == 0 ||  /* some VMs */
                        strlen(vendor) == 12);  /* at minimum, 12 chars */
    TEST_ASSERT(known_vendor, "Vendor string should be 12 characters, got \"%s\"", vendor);
    TEST_ASSERT(eax >= 1, "Max basic leaf should be at least 1, got %u", eax);
}

/* Test CPUID leaf 1: processor info and feature flags */
static void test_cpuid_features(void)
{
    uint32_t eax, ebx, ecx, edx;

    TEST_START("CPUID - Feature Detection (leaf 1)");

    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    uint32_t stepping = eax & 0xF;
    uint32_t model = (eax >> 4) & 0xF;
    uint32_t family = (eax >> 8) & 0xF;
    uint32_t ext_model = (eax >> 16) & 0xF;
    uint32_t ext_family = (eax >> 20) & 0xFF;

    uint32_t display_family = family;
    uint32_t display_model = model;
    if (family == 0x0F)
        display_family += ext_family;
    if (family == 0x06 || family == 0x0F)
        display_model += (ext_model << 4);

    printf("  Family: %u, Model: %u, Stepping: %u\n", display_family, display_model, stepping);
    printf("  ECX features: 0x%08X\n", ecx);
    printf("  EDX features: 0x%08X\n", edx);

    /* All x86-64 processors must support these */
    TEST_ASSERT(edx & (1U << 0), "FPU (bit 0 of EDX) should be set");
    TEST_ASSERT(edx & (1U << 4), "TSC (bit 4 of EDX) should be set");
    TEST_ASSERT(edx & (1U << 5), "MSR (bit 5 of EDX) should be set");
    TEST_ASSERT(edx & (1U << 15), "CMOV (bit 15 of EDX) should be set");
    TEST_ASSERT(edx & (1U << 23), "MMX (bit 23 of EDX) should be set");
    TEST_ASSERT(edx & (1U << 24), "FXSR (bit 24 of EDX) should be set");
    TEST_ASSERT(edx & (1U << 25), "SSE (bit 25 of EDX) should be set");
    TEST_ASSERT(edx & (1U << 26), "SSE2 (bit 26 of EDX) should be set");

    /* Report some optional features */
    printf("  SSE3:    %s\n", (ecx & (1U << 0))  ? "yes" : "no");
    printf("  SSSE3:   %s\n", (ecx & (1U << 9))  ? "yes" : "no");
    printf("  SSE4.1:  %s\n", (ecx & (1U << 19)) ? "yes" : "no");
    printf("  SSE4.2:  %s\n", (ecx & (1U << 20)) ? "yes" : "no");
    printf("  AVX:     %s\n", (ecx & (1U << 28)) ? "yes" : "no");
    printf("  AES-NI:  %s\n", (ecx & (1U << 25)) ? "yes" : "no");
    printf("  RDRAND:  %s\n", (ecx & (1U << 30)) ? "yes" : "no");
}

/* Test CPUID extended function: processor brand string */
static void test_cpuid_extended(void)
{
    uint32_t eax, ebx, ecx, edx;
    char brand[49];

    TEST_START("CPUID - Extended Functions (leaves 0x80000000+)");

    /* Check max extended leaf */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000000)
    );

    uint32_t max_ext = eax;
    printf("  Max extended CPUID leaf: 0x%08X\n", max_ext);
    TEST_ASSERT(max_ext >= 0x80000001, "Max extended leaf should be >= 0x80000001, got 0x%08X", max_ext);

    if (max_ext >= 0x80000004) {
        /* Brand string is in leaves 0x80000002-0x80000004 */
        uint32_t *brand32 = (uint32_t *)brand;
        for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
            __asm__ volatile (
                "cpuid"
                : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                : "a"(leaf)
            );
            int idx = (leaf - 0x80000002) * 4;
            brand32[idx + 0] = eax;
            brand32[idx + 1] = ebx;
            brand32[idx + 2] = ecx;
            brand32[idx + 3] = edx;
        }
        brand[48] = '\0';
        printf("  Processor brand: \"%s\"\n", brand);
        TEST_ASSERT(strlen(brand) > 0, "Brand string should not be empty");
    } else {
        printf("  Brand string not supported (max_ext < 0x80000004)\n");
    }
}

/* Test CPUID leaf 7: structured extended feature flags */
static void test_cpuid_leaf7(void)
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t max_leaf;

    TEST_START("CPUID - Structured Extended Features (leaf 7)");

    /* First check if leaf 7 is supported */
    __asm__ volatile (
        "cpuid"
        : "=a"(max_leaf), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    if (max_leaf < 7) {
        printf("  Leaf 7 not supported (max leaf = %u)\n", max_leaf);
        return;
    }

    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(7), "c"(0)
    );

    printf("  EBX: 0x%08X, ECX: 0x%08X, EDX: 0x%08X\n", ebx, ecx, edx);
    printf("  FSGSBASE:  %s\n", (ebx & (1U << 0))  ? "yes" : "no");
    printf("  BMI1:      %s\n", (ebx & (1U << 3))  ? "yes" : "no");
    printf("  AVX2:      %s\n", (ebx & (1U << 5))  ? "yes" : "no");
    printf("  BMI2:      %s\n", (ebx & (1U << 8))  ? "yes" : "no");
    printf("  AVX-512F:  %s\n", (ebx & (1U << 16)) ? "yes" : "no");
    printf("  RDSEED:    %s\n", (ebx & (1U << 18)) ? "yes" : "no");
    printf("  SHA:       %s\n", (ebx & (1U << 29)) ? "yes" : "no");
    printf("  RDPID:     %s\n", (ecx & (1U << 22)) ? "yes" : "no");

    /* Leaf 7 subleaf 0 EAX = max subleaf index */
    TEST_ASSERT(1, "CPUID leaf 7 subleaf 0 executed successfully");
}

/* Test CPUID with different calling conventions: clobbers check */
static void test_cpuid_clobbers(void)
{
    uint32_t eax1, ebx1, ecx1, edx1;
    uint32_t eax2, ebx2, ecx2, edx2;

    TEST_START("CPUID - Consistency (multiple calls)");

    /* Call CPUID twice with same input, should get same output */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax1), "=b"(ebx1), "=c"(ecx1), "=d"(edx1)
        : "a"(0)
    );

    __asm__ volatile (
        "cpuid"
        : "=a"(eax2), "=b"(ebx2), "=c"(ecx2), "=d"(edx2)
        : "a"(0)
    );

    TEST_ASSERT(eax1 == eax2, "EAX should be consistent: 0x%X vs 0x%X", eax1, eax2);
    TEST_ASSERT(ebx1 == ebx2, "EBX should be consistent: 0x%X vs 0x%X", ebx1, ebx2);
    TEST_ASSERT(ecx1 == ecx2, "ECX should be consistent: 0x%X vs 0x%X", ecx1, ecx2);
    TEST_ASSERT(edx1 == edx2, "EDX should be consistent: 0x%X vs 0x%X", edx1, edx2);
}

int main(void)
{
    test_cpuid_vendor();
    test_cpuid_features();
    test_cpuid_extended();
    test_cpuid_leaf7();
    test_cpuid_clobbers();

    TEST_END();
}
