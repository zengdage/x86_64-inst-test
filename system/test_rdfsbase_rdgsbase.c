/*
 * test_rdfsbase_rdgsbase.c - Test RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE instructions
 *
 * These instructions read/write the FS and GS segment base registers directly
 * without needing a syscall, when the FSGSBASE feature is enabled by the OS.
 *
 * Prerequisites:
 *   - CPUID.07H:EBX[0] (FSGSBASE bit) must be set
 *   - CR4.FSGSBASE must be set by the OS (Linux 5.3+ enables this)
 *   - If CR4.FSGSBASE is not set, these instructions will cause #UD (SIGILL)
 *
 * Compile: gcc -o test_rdfsbase_rdgsbase system/test_rdfsbase_rdgsbase.c -O0
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

static int has_fsgsbase_cpuid(void)
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
    return (ebx >> 0) & 1;
}

/* Check if FSGSBASE instructions are actually usable (CR4.FSGSBASE set) */
static int fsgsbase_enabled(void)
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
        __asm__ volatile ("rdfsbase %0" : "=r"(val));
        (void)val;
    }

    sigaction(SIGILL, &old_sa, NULL);
    return !got_sigill;
}

/* Test RDFSBASE - read FS base (32-bit) */
static void test_rdfsbase32(void)
{
    uint32_t fs_base32;

    TEST_START("RDFSBASE - 32-bit");

    __asm__ volatile (
        "rdfsbase %0"
        : "=r"(fs_base32)
    );

    printf("  FS base (32-bit): 0x%08X\n", fs_base32);
    TEST_ASSERT(1, "RDFSBASE 32-bit executed successfully");
}

/* Test RDFSBASE - read FS base (64-bit) */
static void test_rdfsbase64(void)
{
    uint64_t fs_base64;

    TEST_START("RDFSBASE - 64-bit");

    __asm__ volatile (
        "rdfsbase %0"
        : "=r"(fs_base64)
    );

    printf("  FS base (64-bit): 0x%016" PRIX64 "\n", fs_base64);
    /* On Linux, FS base is typically non-zero (used for TLS) */
    TEST_ASSERT(fs_base64 != 0, "FS base should be non-zero (TLS pointer), got 0x%" PRIX64, fs_base64);
}

/* Test RDGSBASE - read GS base (64-bit) */
static void test_rdgsbase64(void)
{
    uint64_t gs_base64;

    TEST_START("RDGSBASE - 64-bit");

    __asm__ volatile (
        "rdgsbase %0"
        : "=r"(gs_base64)
    );

    printf("  GS base (64-bit): 0x%016" PRIX64 "\n", gs_base64);
    /* GS base may be zero in user space on Linux */
    TEST_ASSERT(1, "RDGSBASE 64-bit executed successfully");
}

/* Test WRFSBASE/RDFSBASE round-trip */
static void test_wrfsbase_rdfsbase_roundtrip(void)
{
    uint64_t original_fs, new_fs, readback;

    TEST_START("WRFSBASE/RDFSBASE - Round-trip");

    /* Save original FS base */
    __asm__ volatile ("rdfsbase %0" : "=r"(original_fs));
    printf("  Original FS base: 0x%016" PRIX64 "\n", original_fs);

    /* Write a new value - use a value close to original to not break TLS */
    /* We write and immediately read back, then restore */
    new_fs = original_fs;  /* safe: write same value */
    __asm__ volatile ("wrfsbase %0" : : "r"(new_fs));

    __asm__ volatile ("rdfsbase %0" : "=r"(readback));
    printf("  After WRFSBASE: 0x%016" PRIX64 "\n", readback);

    TEST_ASSERT(readback == new_fs,
                "RDFSBASE should return written value: expected 0x%" PRIX64 ", got 0x%" PRIX64,
                new_fs, readback);

    /* Restore original */
    __asm__ volatile ("wrfsbase %0" : : "r"(original_fs));
}

/* Test WRGSBASE/RDGSBASE round-trip */
static void test_wrgsbase_rdgsbase_roundtrip(void)
{
    uint64_t original_gs, readback;
    uint64_t test_val;

    TEST_START("WRGSBASE/RDGSBASE - Round-trip");

    /* Save original GS base */
    __asm__ volatile ("rdgsbase %0" : "=r"(original_gs));
    printf("  Original GS base: 0x%016" PRIX64 "\n", original_gs);

    /* Write a test value and read it back.
     * GS base is typically 0 in user-space Linux, so writing to it is safe
     * as long as we restore before any glibc calls that might use it.
     * Must use a canonical address (bits 48-63 must match bit 47) to avoid #GP. */
    test_val = 0x00007FFF12340000ULL;
    __asm__ volatile ("wrgsbase %0" : : "r"(test_val));

    __asm__ volatile ("rdgsbase %0" : "=r"(readback));

    /* Restore original before printf (just in case glibc uses GS) */
    __asm__ volatile ("wrgsbase %0" : : "r"(original_gs));

    printf("  After WRGSBASE(0x%016" PRIX64 "): read back 0x%016" PRIX64 "\n", test_val, readback);

    TEST_ASSERT(readback == test_val,
                "RDGSBASE should return written value: expected 0x%" PRIX64 ", got 0x%" PRIX64,
                test_val, readback);

    /* Restore original */
    __asm__ volatile ("wrgsbase %0" : : "r"(original_gs));

    /* Verify restoration */
    __asm__ volatile ("rdgsbase %0" : "=r"(readback));
    TEST_ASSERT(readback == original_gs,
                "GS base should be restored: expected 0x%" PRIX64 ", got 0x%" PRIX64,
                original_gs, readback);
}

/* Test WRFSBASE with 32-bit operand */
static void test_wrfsbase32(void)
{
    uint64_t original_fs, readback;

    TEST_START("WRFSBASE - 32-bit operand");

    /* Save original */
    __asm__ volatile ("rdfsbase %0" : "=r"(original_fs));

    /*
     * Writing a zero or arbitrary value to FS base would break TLS and crash
     * glibc. Instead, write the same value back using a 32-bit register to
     * verify the instruction form works. We use the low 32 bits of the
     * original FS base (note: 32-bit WRFSBASE zero-extends to 64 bits, so
     * if the original base has bits above 32, this WILL change FS base -- we
     * must restore immediately before calling any glibc function).
     */
    uint32_t test_val32 = (uint32_t)(original_fs & 0xFFFFFFFF);

    __asm__ volatile ("wrfsbase %0" : : "r"(test_val32));
    __asm__ volatile ("rdfsbase %0" : "=r"(readback));

    /* Restore original IMMEDIATELY before any glibc call (printf uses TLS via FS) */
    __asm__ volatile ("wrfsbase %0" : : "r"(original_fs));

    printf("  Wrote 32-bit 0x%08X, read back 64-bit 0x%016" PRIX64 "\n", test_val32, readback);
    TEST_ASSERT(readback == (uint64_t)test_val32,
                "32-bit WRFSBASE should zero-extend: expected 0x%" PRIX64 ", got 0x%" PRIX64,
                (uint64_t)test_val32, readback);
}

int main(void)
{
    printf("Checking FSGSBASE support...\n");

    if (!has_fsgsbase_cpuid()) {
        printf("FSGSBASE not supported by CPU (CPUID.07H:EBX[0] = 0). Skipping all tests.\n");
        TEST_ASSERT(1, "FSGSBASE not available - skip is expected");
        TEST_END();
    }

    printf("  CPUID reports FSGSBASE support\n");

    if (!fsgsbase_enabled()) {
        printf("FSGSBASE instructions cause #UD - CR4.FSGSBASE not enabled by OS. Skipping.\n");
        printf("(Linux 5.3+ should enable this. Current kernel may be older.)\n");
        TEST_ASSERT(1, "FSGSBASE not enabled by OS - skip is expected");
        TEST_END();
    }

    printf("  FSGSBASE instructions are enabled by OS\n\n");

    test_rdfsbase32();
    test_rdfsbase64();
    test_rdgsbase64();
    test_wrfsbase_rdfsbase_roundtrip();
    test_wrgsbase_rdgsbase_roundtrip();
    test_wrfsbase32();

    TEST_END();
}
