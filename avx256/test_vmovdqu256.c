/*
 * Test VMOVDQU (256-bit unaligned integer move), VEX.256.F3.0F 6F/7F
 *
 * Focus: the RIP-relative load form
 *     c5 fe 6f 15 <rel32>    vmovdqu 0x...(%rip),%ymm2
 * VEX 2-byte prefix c5, second byte fe = ~vvvv=1111, L=1 (256-bit), pp=10 (F3),
 * opcode 6f = load, modrm 15 = mod=00 reg=010 (ymm2) rm=101 (RIP+disp32).
 * The 7f opcode is the store direction, and the xmm (L=0) load zeroes the
 * upper 128 bits of the destination ymm register.
 *
 * Compile: gcc -o test_vmovdqu256 avx256/test_vmovdqu256.c -O2 -mavx2
 * Do NOT use static linking.
 */
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(7),"c"(0));
    return (ebx >> 5) & 1;
}
#else
#define check_avx2() 1
#endif

/* Globals live in .rodata/.data and are reached RIP-relative in a PIE build,
 * which is what produces the c5 fe 6f 15 encoding. */
static const uint32_t __attribute__((aligned(32))) g_aligned[8] = {
    0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F,
    0x10111213, 0x14151617, 0x18191A1B, 0x1C1D1E1F
};

/* Deliberately not 32-byte aligned to prove vmovdqu tolerates it. */
static const uint8_t g_unaligned_pad = 0xFF;
static const uint32_t g_unaligned[8] = {
    0xDEADBEEF, 0xCAFEBABE, 0xFEEDFACE, 0x8BADF00D,
    0x0BADCAFE, 0xABAD1DEA, 0xBAAAAAAD, 0xDEFEC8ED
};

static uint32_t g_store_dst[8];

static void test_vmovdqu_rip_load(void) {
    TEST_START("VMOVDQU 256-bit RIP-relative load into ymm2");
    uint32_t dst[8] = {0};

    __asm__ volatile(
        "vmovdqu %1, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_aligned[0])
        : "ymm2", "memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == g_aligned[i],
                    "vmovdqu rip load dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_aligned[i]);
}

static void test_vmovdqu_rip_load_unaligned(void) {
    TEST_START("VMOVDQU 256-bit RIP-relative load, unaligned source");
    uint32_t dst[8] = {0};

    __asm__ volatile(
        "vmovdqu %1, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_unaligned[0])
        : "ymm2", "memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == g_unaligned[i],
                    "vmovdqu unaligned dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_unaligned[i]);
    TEST_ASSERT(g_unaligned_pad == 0xFF, "padding byte untouched");
}

static void test_vmovdqu_rip_store(void) {
    TEST_START("VMOVDQU 256-bit RIP-relative store from ymm2");
    const uint32_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    memset(g_store_dst, 0, sizeof(g_store_dst));
    __asm__ volatile(
        "vmovdqu %1, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0\n\t"
        : "=m"(g_store_dst[0])
        : "m"(src[0])
        : "ymm2", "memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(g_store_dst[i] == src[i],
                    "vmovdqu rip store dst[%d]=%u want %u",
                    i, g_store_dst[i], src[i]);
}

static void test_vmovdqu_reg_reg(void) {
    TEST_START("VMOVDQU 256-bit register to register");
    uint32_t dst[8] = {0};

    __asm__ volatile(
        "vmovdqu %1, %%ymm2\n\t"
        "vmovdqu %%ymm2, %%ymm5\n\t"
        "vpxor %%ymm2, %%ymm2, %%ymm2\n\t"   /* clobber source, dst must survive */
        "vmovdqu %%ymm5, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_aligned[0])
        : "ymm2", "ymm5", "memory"
    );
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(dst[i] == g_aligned[i],
                    "vmovdqu ymm2->ymm5 dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_aligned[i]);
}

static void test_vmovdqu_xmm_zeroes_upper(void) {
    TEST_START("VMOVDQU 128-bit load zeroes upper ymm2 lane");
    uint32_t dst[8] = {0};

    __asm__ volatile(
        "vpcmpeqd %%ymm2, %%ymm2, %%ymm2\n\t" /* fill ymm2 with all ones */
        "vmovdqu %1, %%xmm2\n\t"              /* 128-bit form must zero bits 255:128 */
        "vmovdqu %%ymm2, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_aligned[0])
        : "ymm2", "memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == g_aligned[i],
                    "vmovdqu xmm low dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_aligned[i]);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(dst[i] == 0,
                    "vmovdqu xmm must zero upper lane dst[%d]=0x%08X", i, dst[i]);
}

/* ---- 128-bit (VEX.128.F3.0F 6F/7F) coverage ----
 *
 * Same opcode bytes as 256-bit, only the L bit and the second VEX byte
 * change: c5 fa (L=0) instead of c5 fe (L=1).  Distinct enough that the
 * decoder can desynchronise, so we exercise the form independently.
 */

static void test_vmovdqu128_rip_load(void) {
    TEST_START("VMOVDQU 128-bit RIP-relative load into xmm2");
    uint32_t dst[4] = {0};

    __asm__ volatile(
        "vmovdqu %1, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_aligned[0])
        : "xmm2", "memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == g_aligned[i],
                    "vmovdqu128 rip load dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_aligned[i]);
}

static void test_vmovdqu128_rip_load_unaligned(void) {
    TEST_START("VMOVDQU 128-bit RIP-relative load, unaligned source");
    uint32_t dst[4] = {0};

    __asm__ volatile(
        "vmovdqu %1, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_unaligned[0])
        : "xmm2", "memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == g_unaligned[i],
                    "vmovdqu128 unaligned dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_unaligned[i]);
    /* Sentinel padding byte must remain untouched, proving we read exactly 16
     * bytes from the unaligned source. */
    TEST_ASSERT(g_unaligned_pad == 0xFF, "padding byte untouched");
}

static void test_vmovdqu128_rip_store(void) {
    TEST_START("VMOVDQU 128-bit RIP-relative store from xmm2 (16 bytes only)");
    const uint32_t src[4] = {0xA1A1A1A1, 0xB2B2B2B2, 0xC3C3C3C3, 0xD4D4D4D4};
    /* 32-byte buffer so we can prove the store touches only the low 16. */
    static uint32_t g_store128_dst[8];

    /* Pre-fill high 128 bits with a poison pattern. */
    for (int i = 0; i < 8; i++) g_store128_dst[i] = 0xDEADBEEF;
    __asm__ volatile(
        "vmovdqu %1, %%xmm2\n\t"
        "vmovdqu %%xmm2, %0\n\t"
        : "=m"(g_store128_dst[0])
        : "m"(src[0])
        : "xmm2", "memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(g_store128_dst[i] == src[i],
                    "vmovdqu128 store dst[%d]=0x%08X want 0x%08X",
                    i, g_store128_dst[i], src[i]);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(g_store128_dst[i] == 0xDEADBEEF,
                    "vmovdqu128 store must not touch bytes past 128: "
                    "dst[%d]=0x%08X", i, g_store128_dst[i]);
}

static void test_vmovdqu128_reg_reg(void) {
    TEST_START("VMOVDQU 128-bit register to register (xmm2 -> xmm5)");
    uint32_t dst[4] = {0};

    __asm__ volatile(
        "vmovdqu %1, %%xmm2\n\t"
        "vmovdqu %%xmm2, %%xmm5\n\t"
        "vpxor %%xmm2, %%xmm2, %%xmm2\n\t"   /* clobber source, dst must survive */
        "vmovdqu %%xmm5, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_aligned[0])
        : "xmm2", "xmm5", "memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == g_aligned[i],
                    "vmovdqu128 xmm2->xmm5 dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_aligned[i]);
}

static void test_vmovdqu128_load_zeros_upper_ymm(void) {
    TEST_START("VMOVDQU 128-bit VEX.128 form zeros upper 128 bits of ymm destination");
    /* 32 bytes of poison — load should overwrite low 16 with src and zero
     * bits 255:128 of ymm2, while leaving the storage beyond dst[4] alone. */
    static uint32_t g_poison_dst[8];
    uint32_t dst[8] = {0};

    for (int i = 0; i < 8; i++) g_poison_dst[i] = 0x5A5A5A5A;

    __asm__ volatile(
        /* Fill the destination ymm with a recognisable non-zero pattern first. */
        "vpcmpeqd %%ymm2, %%ymm2, %%ymm2\n\t"
        "vmovdqu %2, %%xmm2\n\t"            /* 128-bit VEX form must zero 255:128 */
        "vmovdqu %%ymm2, %0\n\t"
        : "=m"(dst[0])
        : "m"(g_aligned[0]), "m"(g_aligned[0])
        : "ymm2", "memory"
    );
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(dst[i] == g_aligned[i],
                    "vmovdqu128 low lane dst[%d]=0x%08X want 0x%08X",
                    i, dst[i], g_aligned[i]);
    for (int i = 4; i < 8; i++)
        TEST_ASSERT(dst[i] == 0,
                    "vmovdqu128 must zero upper ymm lane dst[%d]=0x%08X",
                    i, dst[i]);
    /* The unaliased g_poison_dst must be untouched. */
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(g_poison_dst[i] == 0x5A5A5A5A,
                    "g_poison_dst[%d]=0x%08X must be untouched",
                    i, g_poison_dst[i]);
}

int main(void) {
    if (!check_avx2()) { printf("AVX2 not supported\n"); return 1; }
    test_vmovdqu_rip_load();
    test_vmovdqu_rip_load_unaligned();
    test_vmovdqu_rip_store();
    test_vmovdqu_reg_reg();
    test_vmovdqu_xmm_zeroes_upper();
    test_vmovdqu128_rip_load();
    test_vmovdqu128_rip_load_unaligned();
    test_vmovdqu128_rip_store();
    test_vmovdqu128_reg_reg();
    test_vmovdqu128_load_zeros_upper_ymm();
    TEST_END();
}
