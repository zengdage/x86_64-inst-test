/*
 * test_rdpid.c - Test RDPID instruction
 *
 * RDPID reads the value of IA32_TSC_AUX into a general-purpose register.
 * This is the same value that RDTSCP returns in ECX, but RDPID does not
 * read the timestamp counter. On Linux, IA32_TSC_AUX typically contains
 * the logical processor number.
 *
 * Prerequisites:
 *   - CPUID.07H:ECX[22] (RDPID bit) must be set
 *   - If not supported, the instruction will cause #UD (SIGILL)
 *
 * Compile: gcc -o test_rdpid system/test_rdpid.c -O0
 * Note: Do not use static linking (-static)
 */

#include "../common.h"
#include <signal.h>
#include <setjmp.h>

static sigjmp_buf jmp_env;
static volatile int got_sigill = 0;

static void sigill_handler(int sig)
{
    (void)sig;
    got_sigill = 1;
    siglongjmp(jmp_env, 1);
}

static int has_rdpid_cpuid(void)
{
    uint32_t eax, ebx, ecx, edx;
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
    return (ecx >> 22) & 1;
}

static int has_rdtscp(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000001)
    );
    return (edx >> 27) & 1;
}

/* Check if RDPID actually works (may not even if CPUID says so in some VMs) */
static int rdpid_works(void)
{
    struct sigaction sa, old_sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigill_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGILL, &sa, &old_sa);

    got_sigill = 0;
    if (sigsetjmp(jmp_env, 1) == 0) {
        uint64_t val;
        /* RDPID encoding: F3 0F C7 /7 (with reg operand) */
        __asm__ volatile (".byte 0xF3, 0x48, 0x0F, 0xC7, 0xF8" : "=a"(val));
        (void)val;
    }

    sigaction(SIGILL, &old_sa, NULL);
    return !got_sigill;
}

/* Test basic RDPID */
static void test_rdpid_basic(void)
{
    uint64_t pid;

    TEST_START("RDPID - Basic Read");

    /* RDPID r64: F3 0F C7 /7 with REX.W, using RAX as destination */
    __asm__ volatile (
        ".byte 0xF3, 0x48, 0x0F, 0xC7, 0xF8"
        : "=a"(pid)
    );

    printf("  Processor ID (IA32_TSC_AUX): %" PRIu64 " (0x%016" PRIX64 ")\n", pid, pid);
    TEST_ASSERT(1, "RDPID executed successfully");
}

/* Test RDPID consistency - multiple reads on same CPU should match */
static void test_rdpid_consistency(void)
{
    uint64_t pid1, pid2;

    TEST_START("RDPID - Consistency (two consecutive reads)");

    __asm__ volatile (
        ".byte 0xF3, 0x48, 0x0F, 0xC7, 0xF8"
        : "=a"(pid1)
    );

    __asm__ volatile (
        ".byte 0xF3, 0x48, 0x0F, 0xC7, 0xF8"
        : "=a"(pid2)
    );

    printf("  RDPID #1: %" PRIu64 "\n", pid1);
    printf("  RDPID #2: %" PRIu64 "\n", pid2);

    /* Back-to-back reads should return the same value (unlikely to be migrated) */
    TEST_ASSERT(pid1 == pid2,
                "Consecutive RDPID reads should match (no migration expected): %" PRIu64 " vs %" PRIu64,
                pid1, pid2);
}

/* Test RDPID matches RDTSCP's ECX value */
static void test_rdpid_vs_rdtscp(void)
{
    uint64_t rdpid_val;
    uint32_t tsc_lo, tsc_hi, rdtscp_aux;

    TEST_START("RDPID - Compare with RDTSCP aux value");

    if (!has_rdtscp()) {
        printf("  RDTSCP not supported, skipping comparison\n");
        return;
    }

    /* Read both back-to-back to minimize chance of CPU migration */
    __asm__ volatile (
        ".byte 0xF3, 0x48, 0x0F, 0xC7, 0xF8"
        : "=a"(rdpid_val)
    );

    __asm__ volatile (
        "rdtscp"
        : "=a"(tsc_lo), "=d"(tsc_hi), "=c"(rdtscp_aux)
    );

    printf("  RDPID value:     %" PRIu64 "\n", rdpid_val);
    printf("  RDTSCP ECX value: %u\n", rdtscp_aux);

    /* Both should read IA32_TSC_AUX, so they should match if no migration occurred */
    TEST_ASSERT((uint32_t)rdpid_val == rdtscp_aux,
                "RDPID and RDTSCP aux should match: %u vs %u (unless CPU migration occurred)",
                (uint32_t)rdpid_val, rdtscp_aux);
}

int main(void)
{
    printf("Checking RDPID support...\n");

    if (!has_rdpid_cpuid()) {
        printf("RDPID not supported by CPU (CPUID.07H:ECX[22] = 0). Skipping all tests.\n");
        TEST_ASSERT(1, "RDPID not available - skip is expected");
        TEST_END();
    }

    printf("  CPUID reports RDPID support\n");

    if (!rdpid_works()) {
        printf("RDPID instruction caused #UD despite CPUID support. Skipping.\n");
        TEST_ASSERT(1, "RDPID not functional - skip is expected");
        TEST_END();
    }

    printf("  RDPID instruction is functional\n\n");

    test_rdpid_basic();
    test_rdpid_consistency();
    test_rdpid_vs_rdtscp();

    TEST_END();
}
