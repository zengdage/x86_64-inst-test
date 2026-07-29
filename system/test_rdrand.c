/*
 * test_rdrand.c - Test RDRAND and RDSEED instructions
 *
 * RDRAND returns a hardware-generated random number. CF=1 on success, CF=0 on failure.
 * RDSEED returns a random seed value from the entropy source. CF=1 on success, CF=0 on failure.
 * Both support 16-bit, 32-bit, and 64-bit operands.
 *
 * Prerequisites:
 *   RDRAND: CPUID.01H:ECX[30] (RDRAND bit)
 *   RDSEED: CPUID.07H:EBX[18] (RDSEED bit)
 *
 * Compile: gcc -o test_rdrand system/test_rdrand.c -O0
 * Note: Do not use static linking (-static)
 */

#include "../common.h"

static int has_rdrand(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    return (ecx >> 30) & 1;
}

static int has_rdseed(void)
{
    uint32_t eax, ebx, ecx, edx;
    /* Check max leaf first */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    if (eax < 7) return 0;

    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(7), "c"(0)
    );
    return (ebx >> 18) & 1;
}

/* Test RDRAND with 16-bit operand */
static void test_rdrand16(void)
{
    uint16_t val;
    uint8_t cf;

    TEST_START("RDRAND - 16-bit");

    if (!has_rdrand()) {
        printf("  RDRAND not supported, skipping\n");
        return;
    }

    __asm__ volatile (
        "rdrand %1\n\t"
        "setc %0"
        : "=r"(cf), "=r"(val)
    );

    printf("  RDRAND16: 0x%04X (CF=%u)\n", val, cf);
    TEST_ASSERT(cf == 1, "RDRAND16 should succeed (CF=1), got CF=%u", cf);
}

/* Test RDRAND with 32-bit operand */
static void test_rdrand32(void)
{
    uint32_t val;
    uint8_t cf;

    TEST_START("RDRAND - 32-bit");

    if (!has_rdrand()) {
        printf("  RDRAND not supported, skipping\n");
        return;
    }

    __asm__ volatile (
        "rdrand %1\n\t"
        "setc %0"
        : "=r"(cf), "=r"(val)
    );

    printf("  RDRAND32: 0x%08X (CF=%u)\n", val, cf);
    TEST_ASSERT(cf == 1, "RDRAND32 should succeed (CF=1), got CF=%u", cf);
}

/* Test RDRAND with 64-bit operand */
static void test_rdrand64(void)
{
    uint64_t val;
    uint8_t cf;

    TEST_START("RDRAND - 64-bit");

    if (!has_rdrand()) {
        printf("  RDRAND not supported, skipping\n");
        return;
    }

    __asm__ volatile (
        "rdrand %1\n\t"
        "setc %0"
        : "=r"(cf), "=r"(val)
    );

    printf("  RDRAND64: 0x%016" PRIX64 " (CF=%u)\n", val, cf);
    TEST_ASSERT(cf == 1, "RDRAND64 should succeed (CF=1), got CF=%u", cf);
}

/* Test RDRAND produces different values */
static void test_rdrand_uniqueness(void)
{
    uint64_t vals[10];
    uint8_t cf;
    int all_succeeded = 1;
    int all_unique = 1;

    TEST_START("RDRAND - Uniqueness (10 values)");

    if (!has_rdrand()) {
        printf("  RDRAND not supported, skipping\n");
        return;
    }

    for (int i = 0; i < 10; i++) {
        __asm__ volatile (
            "rdrand %1\n\t"
            "setc %0"
            : "=r"(cf), "=r"(vals[i])
        );
        if (cf != 1) all_succeeded = 0;
    }

    /* Check that not all values are the same */
    for (int i = 1; i < 10; i++) {
        if (vals[i] == vals[0]) {
            all_unique = 0;
        }
    }

    printf("  Generated 10 random values:\n");
    for (int i = 0; i < 10; i++) {
        printf("    [%d] 0x%016" PRIX64 "\n", i, vals[i]);
    }

    TEST_ASSERT(all_succeeded, "All 10 RDRAND calls should succeed");
    TEST_ASSERT(all_unique, "Not all 10 values should be identical (extremely unlikely)");
}

/* Test RDSEED with 16-bit operand */
static void test_rdseed16(void)
{
    uint16_t val;
    uint8_t cf;

    TEST_START("RDSEED - 16-bit");

    if (!has_rdseed()) {
        printf("  RDSEED not supported, skipping\n");
        return;
    }

    /* RDSEED may fail if entropy pool is depleted; retry a few times */
    int success = 0;
    for (int attempt = 0; attempt < 10; attempt++) {
        __asm__ volatile (
            "rdseed %1\n\t"
            "setc %0"
            : "=r"(cf), "=r"(val)
        );
        if (cf == 1) {
            success = 1;
            break;
        }
    }

    printf("  RDSEED16: 0x%04X (success=%d)\n", val, success);
    TEST_ASSERT(success, "RDSEED16 should succeed within 10 attempts");
}

/* Test RDSEED with 32-bit operand */
static void test_rdseed32(void)
{
    uint32_t val;
    uint8_t cf;

    TEST_START("RDSEED - 32-bit");

    if (!has_rdseed()) {
        printf("  RDSEED not supported, skipping\n");
        return;
    }

    int success = 0;
    for (int attempt = 0; attempt < 10; attempt++) {
        __asm__ volatile (
            "rdseed %1\n\t"
            "setc %0"
            : "=r"(cf), "=r"(val)
        );
        if (cf == 1) {
            success = 1;
            break;
        }
    }

    printf("  RDSEED32: 0x%08X (success=%d)\n", val, success);
    TEST_ASSERT(success, "RDSEED32 should succeed within 10 attempts");
}

/* Test RDSEED with 64-bit operand */
static void test_rdseed64(void)
{
    uint64_t val;
    uint8_t cf;

    TEST_START("RDSEED - 64-bit");

    if (!has_rdseed()) {
        printf("  RDSEED not supported, skipping\n");
        return;
    }

    int success = 0;
    for (int attempt = 0; attempt < 10; attempt++) {
        __asm__ volatile (
            "rdseed %1\n\t"
            "setc %0"
            : "=r"(cf), "=r"(val)
        );
        if (cf == 1) {
            success = 1;
            break;
        }
    }

    printf("  RDSEED64: 0x%016" PRIX64 " (success=%d)\n", val, success);
    TEST_ASSERT(success, "RDSEED64 should succeed within 10 attempts");
}

int main(void)
{
    test_rdrand16();
    test_rdrand32();
    test_rdrand64();
    test_rdrand_uniqueness();
    test_rdseed16();
    test_rdseed32();
    test_rdseed64();

    TEST_END();
}
