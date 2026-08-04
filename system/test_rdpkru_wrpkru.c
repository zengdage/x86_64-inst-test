/*
 * test_rdpkru_wrpkru.c - Test RDPKRU and WRPKRU instructions
 *
 * RDPKRU reads the PKRU (Protection Key Rights Register for User pages)
 * into EAX (ECX and EDX are zeroed). ECX must be 0 on input.
 * WRPKRU writes EAX to PKRU. ECX and EDX must be 0 on input.
 *
 * The PKRU register controls access rights for user-mode pages tagged
 * with protection keys. Bits [2i+1:2i] control key i:
 *   Bit 2i:   AD (Access Disable) - disables all access
 *   Bit 2i+1: WD (Write Disable) - disables write access
 *
 * Prerequisites:
 *   - CPUID.07H:ECX[3] (PKU / OSPKE bit) must be set
 *   - CR4.PKE must be set by the OS
 *   - Linux 4.6+ with CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS
 *
 * Compile: gcc -o test_rdpkru_wrpkru system/test_rdpkru_wrpkru.c -O0
 * Note: Do not use static linking (-static)
 */

#include "../common.h"
#include <signal.h>
#include <setjmp.h>

#if ENABLE_RUNTIME_CPU_CHECKS
static sigjmp_buf jmp_env;
static volatile int got_sigill = 0;

static void sigill_handler(int sig)
{
    (void)sig;
    got_sigill = 1;
    siglongjmp(jmp_env, 1);
}

static int has_pku_cpuid(void)
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
    return (ecx >> 3) & 1;
}

/* Check if OSPKE is set (OS has enabled PKU) by checking CPUID.07H:ECX[4] */
static int has_ospke(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(7), "c"(0)
    );
    return (ecx >> 4) & 1;
}

/* Try executing RDPKRU to see if it works */
static int rdpkru_works(void)
{
    struct sigaction sa, old_sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigill_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGILL, &sa, &old_sa);

    got_sigill = 0;
    if (sigsetjmp(jmp_env, 1) == 0) {
        uint32_t pkru;
        __asm__ volatile (
            "xor %%ecx, %%ecx\n\t"
            "rdpkru"
            : "=a"(pkru)
            :
            : "ecx", "edx"
        );
        (void)pkru;
    }

    sigaction(SIGILL, &old_sa, NULL);
    return !got_sigill;
}
#else
#define has_pku_cpuid() 1
#define has_ospke() 1
#define rdpkru_works() 1
#endif

/* Test basic RDPKRU */
static void test_rdpkru_basic(void)
{
    uint64_t pkru64;
    uint32_t edx_out;

    TEST_START("RDPKRU - Basic read");

    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(pkru64), "=d"(edx_out)
        :
        : "ecx"
    );

    uint32_t pkru = (uint32_t)pkru64;

    printf("  PKRU = 0x%08X\n", pkru);

    /* Decode protection key rights */
    for (int key = 0; key < 16; key++) {
        int ad = (pkru >> (2 * key)) & 1;
        int wd = (pkru >> (2 * key + 1)) & 1;
        if (ad || wd) {
            printf("  Key %2d: AD=%d WD=%d\n", key, ad, wd);
        }
    }

    /* Key 0 is typically used for normal allocations; AD and WD should be 0 */
    TEST_ASSERT(((pkru >> 0) & 3) == 0,
                "Key 0 should have no restrictions (bits[1:0] = 0), got %u", pkru & 3);
    TEST_ASSERT((pkru64 >> 32) == 0, "RDPKRU zero-extends EAX into RAX");
    TEST_ASSERT(edx_out == 0, "RDPKRU clears EDX");
}

/* Test WRPKRU/RDPKRU round-trip */
static void test_wrpkru_rdpkru_roundtrip(void)
{
    uint32_t original_pkru, new_pkru, readback;

    TEST_START("WRPKRU/RDPKRU - Round-trip");

    /* Read original PKRU */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(original_pkru)
        :
        : "ecx", "edx"
    );
    printf("  Original PKRU: 0x%08X\n", original_pkru);

    /* Write a modified value: set WD (write-disable) for key 15 */
    /* Key 15 bits are [31:30]. Set bit 31 (WD for key 15). */
    new_pkru = original_pkru | (1U << 31);
    printf("  Writing PKRU: 0x%08X (set WD for key 15)\n", new_pkru);

    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "xor %%edx, %%edx\n\t"
        "wrpkru"
        :
        : "a"(new_pkru)
        : "ecx", "edx"
    );

    /* Read back */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(readback)
        :
        : "ecx", "edx"
    );
    printf("  Read back PKRU: 0x%08X\n", readback);

    TEST_ASSERT(readback == new_pkru,
                "PKRU should match written value: expected 0x%08X, got 0x%08X",
                new_pkru, readback);

    /* Restore original */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "xor %%edx, %%edx\n\t"
        "wrpkru"
        :
        : "a"(original_pkru)
        : "ecx", "edx"
    );

    /* Verify restoration */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(readback)
        :
        : "ecx", "edx"
    );

    TEST_ASSERT(readback == original_pkru,
                "PKRU should be restored: expected 0x%08X, got 0x%08X",
                original_pkru, readback);
}

/* Test WRPKRU with zero value (all keys allow full access) */
static void test_wrpkru_zero(void)
{
    uint32_t original_pkru, readback;

    TEST_START("WRPKRU - Write zero (all access allowed)");

    /* Save original */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(original_pkru)
        :
        : "ecx", "edx"
    );

    /* Write 0 */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "xor %%edx, %%edx\n\t"
        "wrpkru"
        :
        : "a"((uint32_t)0)
        : "ecx", "edx"
    );

    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(readback)
        :
        : "ecx", "edx"
    );

    printf("  PKRU after writing 0: 0x%08X\n", readback);
    TEST_ASSERT(readback == 0, "PKRU should be 0 after writing 0, got 0x%08X", readback);

    /* Restore original */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "xor %%edx, %%edx\n\t"
        "wrpkru"
        :
        : "a"(original_pkru)
        : "ecx", "edx"
    );
}

/* Test WRPKRU with all bits set (maximum restriction) */
static void test_wrpkru_all_bits(void)
{
    uint32_t original_pkru, readback;

    TEST_START("WRPKRU - Write 0xFFFFFFFC (restrict keys 1-15, leave key 0)");

    /* Save original */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(original_pkru)
        :
        : "ecx", "edx"
    );

    /*
     * Set AD+WD for all keys except key 0 to avoid disrupting normal memory access.
     * Key 0 bits are [1:0], so mask = 0xFFFFFFFC.
     */
    uint32_t test_val = 0xFFFFFFFC;

    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "xor %%edx, %%edx\n\t"
        "wrpkru"
        :
        : "a"(test_val)
        : "ecx", "edx"
    );

    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "rdpkru"
        : "=a"(readback)
        :
        : "ecx", "edx"
    );

    printf("  PKRU after writing 0x%08X: 0x%08X\n", test_val, readback);
    TEST_ASSERT(readback == test_val,
                "PKRU should be 0x%08X, got 0x%08X", test_val, readback);

    /* Restore original */
    __asm__ volatile (
        "xor %%ecx, %%ecx\n\t"
        "xor %%edx, %%edx\n\t"
        "wrpkru"
        :
        : "a"(original_pkru)
        : "ecx", "edx"
    );
}

/* Test RDPKRU does not modify flags */
static void test_rdpkru_no_flag_change(void)
{
    uint64_t flags_before, flags_after;
    uint32_t pkru;

    TEST_START("RDPKRU - Does not modify flags");

    __asm__ volatile (
        "movl $0, %%ecx\n\t"
        "movq $0x8d5, %%r11\n\t"
        "pushq %%r11\n\t"
        "popfq\n\t"
        "pushfq\n\t"
        "popq %1\n\t"
        "rdpkru\n\t"
        "pushfq\n\t"
        "popq %2"
        : "=a"(pkru), "=&r"(flags_before), "=&r"(flags_after)
        :
        : "ecx", "edx", "r11", "cc"
    );
    (void)pkru;
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "RDPKRU preserves flags: before=%#" PRIx64 " after=%#" PRIx64,
                flags_before & mask, flags_after & mask);
}

static sigjmp_buf pkru_fault_env;
static volatile sig_atomic_t got_pkru_fault;

static void pkru_fault_handler(int sig)
{
    (void)sig;
    got_pkru_fault = 1;
    siglongjmp(pkru_fault_env, 1);
}

static void test_pkru_reserved_inputs(void)
{
    uint32_t original, readback;
    struct sigaction sa, old_segv, old_bus;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = pkru_fault_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old_segv);
    sigaction(SIGBUS, &sa, &old_bus);

    __asm__ volatile ("movl $0, %%ecx\n\trdpkru" : "=a"(original) : : "ecx", "edx");

    for (int which = 0; which < 3; which++) {
        got_pkru_fault = 0;
        if (sigsetjmp(pkru_fault_env, 1) == 0) {
            if (which == 0) {
                __asm__ volatile ("movl $1, %%ecx\n\trdpkru" : : : "eax", "ecx", "edx");
            } else if (which == 1) {
                __asm__ volatile ("movl $1, %%ecx\n\tmovl $0, %%edx\n\twrpkru"
                                  : : "a"(original) : "ecx", "edx");
            } else {
                __asm__ volatile ("movl $0, %%ecx\n\tmovl $1, %%edx\n\twrpkru"
                                  : : "a"(original) : "ecx", "edx");
            }
        }
        TEST_ASSERT(got_pkru_fault, "%s with reserved nonzero input raises #GP",
                    which == 0 ? "RDPKRU ECX" : (which == 1 ? "WRPKRU ECX" : "WRPKRU EDX"));
        __asm__ volatile ("movl $0, %%ecx\n\trdpkru" : "=a"(readback) : : "ecx", "edx");
        TEST_ASSERT(readback == original, "PKRU unchanged after reserved-input fault case %d", which);
    }

    sigaction(SIGSEGV, &old_segv, NULL);
    sigaction(SIGBUS, &old_bus, NULL);
}

int main(void)
{
    printf("Checking PKU (Protection Keys for User pages) support...\n");

    if (!has_pku_cpuid()) {
        printf("PKU not supported by CPU (CPUID.07H:ECX[3] = 0). Skipping all tests.\n");
        TEST_ASSERT(1, "PKU not available - skip is expected");
        TEST_END();
    }

    printf("  CPUID reports PKU support\n");

    if (!has_ospke()) {
        printf("OSPKE not set (CPUID.07H:ECX[4] = 0). OS has not enabled PKU. Skipping.\n");
        TEST_ASSERT(1, "PKU not enabled by OS - skip is expected");
        TEST_END();
    }

    printf("  OSPKE is set (OS has enabled PKU)\n");

    if (!rdpkru_works()) {
        printf("RDPKRU instruction caused #UD. Skipping.\n");
        TEST_ASSERT(1, "RDPKRU not functional - skip is expected");
        TEST_END();
    }

    printf("  RDPKRU instruction is functional\n\n");

    test_rdpkru_basic();
    test_wrpkru_rdpkru_roundtrip();
    test_wrpkru_zero();
    test_wrpkru_all_bits();
    test_rdpkru_no_flag_change();
    test_pkru_reserved_inputs();

    TEST_END();
}
