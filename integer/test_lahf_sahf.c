/*
 * test_lahf_sahf.c - Test x86-64 LAHF/SAHF instructions
 *
 * LAHF: Loads AH with the low byte of RFLAGS (SF:ZF:0:AF:0:PF:1:CF)
 * SAHF: Stores AH into the low byte of RFLAGS
 * Note: LAHF/SAHF may not be available on all x86-64 CPUs (CPUID check needed)
 *
 * Compile: gcc -o test_lahf_sahf integer/test_lahf_sahf.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_lahf(void) {
    uint8_t ah_val;

    /* Set CF and ZF, then LAHF */
    __asm__ volatile (
        "xorl %%eax, %%eax\n\t"    /* ZF=1 */
        "stc\n\t"                    /* CF=1 */
        "lahf\n\t"
        "movb %%ah, %0"
        : "=r"(ah_val)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(ah_val & 0x01, "lahf: CF bit should be set");
    TEST_ASSERT(ah_val & 0x40, "lahf: ZF bit should be set");

    /* Clear all, check */
    __asm__ volatile (
        "movq $1, %%rax\n\t"
        "testq %%rax, %%rax\n\t"   /* clears CF, ZF=0, SF=0 */
        "lahf\n\t"
        "movb %%ah, %0"
        : "=r"(ah_val)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(!(ah_val & 0x01), "lahf: CF bit should be clear");
    TEST_ASSERT(!(ah_val & 0x40), "lahf: ZF bit should be clear");
    TEST_ASSERT(!(ah_val & 0x80), "lahf: SF bit should be clear");
}

static void test_sahf(void) {
    uint64_t flags;

    /* Set specific flags via SAHF */
    /* AH bit layout: SF:ZF:0:AF:0:PF:1:CF */
    /* Set CF=1, ZF=1: AH = 0b01000111 = 0x47 */
    __asm__ volatile (
        "movb $0x47, %%ah\n\t"
        "sahf\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "sahf: CF should be set");
    TEST_ASSERT(flags & ZF_FLAG, "sahf: ZF should be set");
    TEST_ASSERT(flags & PF_FLAG, "sahf: PF should be set");

    /* Clear all via SAHF: AH = 0b00000010 = 0x02 (bit 1 always set) */
    __asm__ volatile (
        "movb $0x02, %%ah\n\t"
        "sahf\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "rax", "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "sahf clear: CF should be clear");
    TEST_ASSERT(!(flags & ZF_FLAG), "sahf clear: ZF should be clear");
    TEST_ASSERT(!(flags & SF_FLAG), "sahf clear: SF should be clear");
}

static void test_lahf_sahf_roundtrip(void) {
    uint8_t ah_saved;
    uint64_t flags_before, flags_after;

    /* Save flags, modify, restore */
    __asm__ volatile (
        "stc\n\t"
        "lahf\n\t"
        "movzbl %%ah, %%edx\n\t"
        "movb %%dl, %0\n\t"
        "pushfq\n\t"
        "popq %1\n\t"
        "clc\n\t"                /* modify CF */
        "movb %0, %%dl\n\t"
        "movb %%dl, %%ah\n\t"
        "sahf\n\t"              /* restore */
        "pushfq\n\t"
        "popq %2"
        : "=m"(ah_saved), "=r"(flags_before), "=r"(flags_after)
        :
        : "rax", "rdx", "cc"
    );
    /* Low byte flags should match */
    uint64_t mask = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG;
    TEST_ASSERT((flags_before & mask) == (flags_after & mask),
                "lahf/sahf roundtrip: flags restored");
}

int main(void) {
    TEST_START("LAHF/SAHF instructions");
    test_lahf();
    test_sahf();
    test_lahf_sahf_roundtrip();
    TEST_END();
}
