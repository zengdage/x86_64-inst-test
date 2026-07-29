/*
 * test_rdrand_rdseed.c - Test x86-64 RDRAND and RDSEED instructions
 *
 * RDRAND generates a random number from an on-chip hardware random
 * number generator. Sets CF=1 on success, CF=0 on failure (underflow).
 *
 * RDSEED generates a random seed value from the hardware entropy source.
 * Sets CF=1 on success, CF=0 on failure (no entropy available).
 *
 * Both support 16-bit, 32-bit, and 64-bit operand sizes.
 * Requires RDRAND/RDSEED support (check CPUID).
 *
 * Compile: gcc -o test_rdrand_rdseed crypto/test_rdrand_rdseed.c -O0 -mrdrnd -mrdseed
 * Note: Do not use static linking.
 */
#include "../common.h"

/* Check CPUID for RDRAND support (CPUID.01H:ECX.RDRAND[bit 30]) */
static int has_rdrand(void) {
    uint32_t ecx;
    __asm__ volatile (
        "movl $1, %%eax\n\t"
        "cpuid"
        : "=c"(ecx)
        :
        : "eax", "ebx", "edx"
    );
    return (ecx >> 30) & 1;
}

/* Check CPUID for RDSEED support (CPUID.07H:EBX.RDSEED[bit 18]) */
static int has_rdseed(void) {
    uint32_t ebx;
    __asm__ volatile (
        "movl $7, %%eax\n\t"
        "xorl %%ecx, %%ecx\n\t"
        "cpuid"
        : "=b"(ebx)
        :
        : "eax", "ecx", "edx"
    );
    return (ebx >> 18) & 1;
}

/* Test RDRAND 16-bit */
static void test_rdrand16(void) {
    uint16_t val;
    uint8_t success;

    __asm__ volatile (
        "rdrand %%ax\n\t"
        "setc %1\n\t"
        "movw %%ax, %0"
        : "=r"(val), "=r"(success)
        :
        : "ax"
    );

    if (success) {
        /* Get another value to check they differ (extremely high probability) */
        uint16_t val2;
        uint8_t success2;
        int differs = 0;

        /* Try a few times since 16-bit has small range */
        for (int i = 0; i < 10; i++) {
            __asm__ volatile (
                "rdrand %%ax\n\t"
                "setc %1\n\t"
                "movw %%ax, %0"
                : "=r"(val2), "=r"(success2)
                :
                : "ax"
            );
            if (success2 && val2 != val) {
                differs = 1;
                break;
            }
        }
        TEST_ASSERT(differs, "rdrand16: produces varying values");
    } else {
        /* RDRAND can fail if entropy is exhausted; not a test failure */
        printf("  NOTE: rdrand16 returned CF=0 (entropy exhausted)\n");
        test_passed++;
    }
}

/* Test RDRAND 32-bit */
static void test_rdrand32(void) {
    uint32_t val;
    uint8_t success;

    __asm__ volatile (
        "rdrand %%eax\n\t"
        "setc %1\n\t"
        "movl %%eax, %0"
        : "=r"(val), "=r"(success)
        :
        : "eax"
    );

    if (success) {
        uint32_t val2;
        uint8_t success2;
        __asm__ volatile (
            "rdrand %%eax\n\t"
            "setc %1\n\t"
            "movl %%eax, %0"
            : "=r"(val2), "=r"(success2)
            :
            : "eax"
        );

        if (success2) {
            TEST_ASSERT(val != val2,
                        "rdrand32: two calls produce different values "
                        "(0x%08x vs 0x%08x)", val, val2);
        } else {
            printf("  NOTE: second rdrand32 returned CF=0\n");
            test_passed++;
        }
    } else {
        printf("  NOTE: rdrand32 returned CF=0 (entropy exhausted)\n");
        test_passed++;
    }
}

/* Test RDRAND 64-bit */
static void test_rdrand64(void) {
    uint64_t val;
    uint8_t success;

    __asm__ volatile (
        "rdrand %%rax\n\t"
        "setc %1\n\t"
        "movq %%rax, %0"
        : "=r"(val), "=r"(success)
        :
        : "rax"
    );

    if (success) {
        uint64_t val2;
        uint8_t success2;
        __asm__ volatile (
            "rdrand %%rax\n\t"
            "setc %1\n\t"
            "movq %%rax, %0"
            : "=r"(val2), "=r"(success2)
            :
            : "rax"
        );

        if (success2) {
            TEST_ASSERT(val != val2,
                        "rdrand64: two calls produce different values");
        } else {
            printf("  NOTE: second rdrand64 returned CF=0\n");
            test_passed++;
        }
    } else {
        printf("  NOTE: rdrand64 returned CF=0 (entropy exhausted)\n");
        test_passed++;
    }
}

/* Test RDRAND multiple calls produce different values (statistical test) */
static void test_rdrand_randomness(void) {
    uint64_t values[8];
    int successes = 0;

    for (int i = 0; i < 8; i++) {
        uint8_t ok;
        __asm__ volatile (
            "rdrand %%rax\n\t"
            "setc %1\n\t"
            "movq %%rax, %0"
            : "=r"(values[i]), "=r"(ok)
            :
            : "rax"
        );
        if (ok) successes++;
    }

    if (successes >= 2) {
        /* Check that not all successful values are the same */
        int all_same = 1;
        for (int i = 1; i < 8; i++) {
            if (values[i] != values[0]) {
                all_same = 0;
                break;
            }
        }
        TEST_ASSERT(!all_same, "rdrand64: multiple calls produce varying values");
    } else {
        printf("  NOTE: insufficient rdrand successes for randomness test\n");
        test_passed++;
    }
}

/* Test RDRAND CF flag behavior with retry loop */
static void test_rdrand_retry(void) {
    uint64_t val;
    int success = 0;

    /* Retry up to 10 times (standard practice for RDRAND) */
    for (int i = 0; i < 10; i++) {
        uint8_t ok;
        __asm__ volatile (
            "rdrand %%rax\n\t"
            "setc %1\n\t"
            "movq %%rax, %0"
            : "=r"(val), "=r"(ok)
            :
            : "rax"
        );
        if (ok) {
            success = 1;
            break;
        }
    }

    TEST_ASSERT(success, "rdrand: succeeds within 10 retries");
}

/* Test RDSEED 16-bit */
static void test_rdseed16(void) {
    uint16_t val;
    uint8_t success = 0;

    /* RDSEED may fail more often than RDRAND, retry a few times */
    for (int i = 0; i < 20; i++) {
        __asm__ volatile (
            "rdseed %%ax\n\t"
            "setc %1\n\t"
            "movw %%ax, %0"
            : "=r"(val), "=r"(success)
            :
            : "ax"
        );
        if (success) break;
    }

    if (success) {
        uint16_t val2;
        uint8_t success2 = 0;
        int differs = 0;

        for (int i = 0; i < 20; i++) {
            __asm__ volatile (
                "rdseed %%ax\n\t"
                "setc %1\n\t"
                "movw %%ax, %0"
                : "=r"(val2), "=r"(success2)
                :
                : "ax"
            );
            if (success2 && val2 != val) {
                differs = 1;
                break;
            }
        }
        TEST_ASSERT(differs, "rdseed16: produces varying values");
    } else {
        printf("  NOTE: rdseed16 failed after retries (no entropy)\n");
        test_passed++;
    }
}

/* Test RDSEED 32-bit */
static void test_rdseed32(void) {
    uint32_t val;
    uint8_t success = 0;

    for (int i = 0; i < 20; i++) {
        __asm__ volatile (
            "rdseed %%eax\n\t"
            "setc %1\n\t"
            "movl %%eax, %0"
            : "=r"(val), "=r"(success)
            :
            : "eax"
        );
        if (success) break;
    }

    if (success) {
        uint32_t val2;
        uint8_t success2 = 0;
        for (int i = 0; i < 20; i++) {
            __asm__ volatile (
                "rdseed %%eax\n\t"
                "setc %1\n\t"
                "movl %%eax, %0"
                : "=r"(val2), "=r"(success2)
                :
                : "eax"
            );
            if (success2 && val2 != val) break;
        }

        if (success2) {
            TEST_ASSERT(val != val2,
                        "rdseed32: two calls produce different values");
        } else {
            printf("  NOTE: second rdseed32 failed\n");
            test_passed++;
        }
    } else {
        printf("  NOTE: rdseed32 failed after retries\n");
        test_passed++;
    }
}

/* Test RDSEED 64-bit */
static void test_rdseed64(void) {
    uint64_t val;
    uint8_t success = 0;

    for (int i = 0; i < 20; i++) {
        __asm__ volatile (
            "rdseed %%rax\n\t"
            "setc %1\n\t"
            "movq %%rax, %0"
            : "=r"(val), "=r"(success)
            :
            : "rax"
        );
        if (success) break;
    }

    if (success) {
        uint64_t val2;
        uint8_t success2 = 0;
        for (int i = 0; i < 20; i++) {
            __asm__ volatile (
                "rdseed %%rax\n\t"
                "setc %1\n\t"
                "movq %%rax, %0"
                : "=r"(val2), "=r"(success2)
                :
                : "rax"
            );
            if (success2 && val2 != val) break;
        }

        if (success2) {
            TEST_ASSERT(val != val2,
                        "rdseed64: two calls produce different values");
        } else {
            printf("  NOTE: second rdseed64 failed\n");
            test_passed++;
        }
    } else {
        printf("  NOTE: rdseed64 failed after retries\n");
        test_passed++;
    }
}

/* Test RDRAND output uses full register width */
static void test_rdrand_full_width(void) {
    int has_high_bits = 0;

    for (int i = 0; i < 100; i++) {
        uint64_t val;
        uint8_t ok;
        __asm__ volatile (
            "rdrand %%rax\n\t"
            "setc %1\n\t"
            "movq %%rax, %0"
            : "=r"(val), "=r"(ok)
            :
            : "rax"
        );
        if (ok && (val >> 32) != 0) {
            has_high_bits = 1;
            break;
        }
    }

    TEST_ASSERT(has_high_bits,
                "rdrand64: produces values with high 32 bits set");
}

int main(void) {
    TEST_START("RDRAND/RDSEED instructions");

    if (has_rdrand()) {
        test_rdrand16();
        test_rdrand32();
        test_rdrand64();
        test_rdrand_randomness();
        test_rdrand_retry();
        test_rdrand_full_width();
    } else {
        printf("  SKIP: RDRAND not supported on this CPU\n");
    }

    if (has_rdseed()) {
        test_rdseed16();
        test_rdseed32();
        test_rdseed64();
    } else {
        printf("  SKIP: RDSEED not supported on this CPU\n");
    }

    TEST_END();
}
