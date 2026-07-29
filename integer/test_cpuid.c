/*
 * test_cpuid.c - Test x86-64 CPUID instruction
 *
 * CPUID returns processor identification and feature information.
 * Input: EAX (and sometimes ECX) selects the information to return.
 * Output: EAX, EBX, ECX, EDX contain the requested info.
 *
 * Compile: gcc -o test_cpuid integer/test_cpuid.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_cpuid_vendor(void) {
    uint32_t eax, ebx, ecx, edx;

    /* CPUID leaf 0: vendor string */
    __asm__ volatile (
        "movl $0, %%eax\n\t"
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
        : "cc"
    );

    /* EAX = max basic leaf supported */
    TEST_ASSERT(eax >= 1, "cpuid leaf 0: max leaf >= 1, got %u", eax);

    /* Vendor string is EBX:EDX:ECX (12 bytes) */
    char vendor[13];
    *(uint32_t *)(vendor + 0) = ebx;
    *(uint32_t *)(vendor + 4) = edx;
    *(uint32_t *)(vendor + 8) = ecx;
    vendor[12] = '\0';

    /* Should be one of known vendors */
    int valid = (strcmp(vendor, "GenuineIntel") == 0 ||
                 strcmp(vendor, "AuthenticAMD") == 0 ||
                 strcmp(vendor, "HygonGenuine") == 0 ||
                 strlen(vendor) == 12);  /* accept any 12-char string */
    TEST_ASSERT(valid, "cpuid vendor: got '%s'", vendor);
    printf("  Vendor: %s\n", vendor);
}

static void test_cpuid_features(void) {
    uint32_t eax, ebx, ecx, edx;

    /* CPUID leaf 1: feature flags */
    __asm__ volatile (
        "movl $1, %%eax\n\t"
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
        : "cc"
    );

    /* Check some common features that should be present on x86-64 */
    TEST_ASSERT(edx & (1 << 0), "cpuid: FPU should be present");
    TEST_ASSERT(edx & (1 << 4), "cpuid: TSC should be present");
    TEST_ASSERT(edx & (1 << 15), "cpuid: CMOV should be present");
    TEST_ASSERT(edx & (1 << 25), "cpuid: SSE should be present");
    TEST_ASSERT(edx & (1 << 26), "cpuid: SSE2 should be present");

    printf("  Stepping: %u, Model: %u, Family: %u\n",
           eax & 0xF, (eax >> 4) & 0xF, (eax >> 8) & 0xF);
}

static void test_cpuid_extended(void) {
    uint32_t eax, ebx, ecx, edx;

    /* CPUID leaf 0x80000000: max extended leaf */
    __asm__ volatile (
        "movl $0x80000000, %%eax\n\t"
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
        : "cc"
    );
    TEST_ASSERT(eax >= 0x80000001, "cpuid: extended leaves supported, max=0x%x", eax);

    /* If brand string is available, read it */
    if (eax >= 0x80000004) {
        char brand[49] = {0};
        for (uint32_t i = 0; i < 3; i++) {
            __asm__ volatile (
                "cpuid"
                : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                : "a"(0x80000002 + i)
                : "cc"
            );
            *(uint32_t *)(brand + i*16 + 0) = eax;
            *(uint32_t *)(brand + i*16 + 4) = ebx;
            *(uint32_t *)(brand + i*16 + 8) = ecx;
            *(uint32_t *)(brand + i*16 + 12) = edx;
        }
        brand[48] = '\0';
        printf("  Brand: %s\n", brand);
        TEST_ASSERT(strlen(brand) > 0, "cpuid: brand string non-empty");
    }
}

int main(void) {
    TEST_START("CPUID instruction");
    test_cpuid_vendor();
    test_cpuid_features();
    test_cpuid_extended();
    TEST_END();
}
