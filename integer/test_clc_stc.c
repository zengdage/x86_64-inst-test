/*
 * test_clc_stc.c - Test x86-64 CLC/STC/CMC/CLD/STD instructions
 *
 * CLC: Clear carry flag
 * STC: Set carry flag
 * CMC: Complement (toggle) carry flag
 * CLD: Clear direction flag (forward string operations)
 * STD: Set direction flag (backward string operations)
 *
 * Compile: gcc -o test_clc_stc integer/test_clc_stc.c -O0
 * Note: Do not use static linking.
 */
#include "../common.h"

#define DF_FLAG (1UL << 10)

static void test_clc(void) {
    uint64_t flags;

    __asm__ volatile (
        "stc\n\t"
        "clc\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "clc: CF should be clear");
}

static void test_stc(void) {
    uint64_t flags;

    __asm__ volatile (
        "clc\n\t"
        "stc\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "stc: CF should be set");
}

static void test_cmc(void) {
    uint64_t flags;

    /* CMC: 0 -> 1 */
    __asm__ volatile (
        "clc\n\t"
        "cmc\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "cmc (0->1): CF should be set");

    /* CMC: 1 -> 0 */
    __asm__ volatile (
        "stc\n\t"
        "cmc\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "cc"
    );
    TEST_ASSERT(!(flags & CF_FLAG), "cmc (1->0): CF should be clear");

    /* CMC twice = identity */
    __asm__ volatile (
        "stc\n\t"
        "cmc\n\t"
        "cmc\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "cc"
    );
    TEST_ASSERT(flags & CF_FLAG, "cmc twice: CF back to original (set)");
}

static void test_cld(void) {
    uint64_t flags;

    __asm__ volatile (
        "std\n\t"
        "cld\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "cc"
    );
    TEST_ASSERT(!(flags & DF_FLAG), "cld: DF should be clear");
}

static void test_std(void) {
    uint64_t flags;

    __asm__ volatile (
        "cld\n\t"
        "std\n\t"
        "pushfq\n\t"
        "popq %0\n\t"
        "cld"              /* restore DF=0 for ABI compliance */
        : "=r"(flags)
        :
        : "cc"
    );
    TEST_ASSERT(flags & DF_FLAG, "std: DF should be set");
}

static void test_flag_instruction_isolation(void) {
    uint64_t before, after;
    uint64_t status_without_cf = PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;

    __asm__ volatile (
        "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "pushfq\n\t" "popq %0\n\t" "clc\n\t" "pushfq\n\t" "popq %1"
        : "=&r"(before), "=&r"(after) : : "r11", "cc");
    TEST_ASSERT(!(after & CF_FLAG), "clc clears only CF");
    TEST_ASSERT((before & status_without_cf) == (after & status_without_cf),
                "clc preserves PF/AF/ZF/SF/OF");

    __asm__ volatile (
        "movq $0x8d4, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "pushfq\n\t" "popq %0\n\t" "stc\n\t" "pushfq\n\t" "popq %1"
        : "=&r"(before), "=&r"(after) : : "r11", "cc");
    TEST_ASSERT(after & CF_FLAG, "stc sets only CF");
    TEST_ASSERT((before & status_without_cf) == (after & status_without_cf),
                "stc preserves PF/AF/ZF/SF/OF");

    __asm__ volatile (
        "movq $0x8d5, %%r11\n\t" "pushq %%r11\n\t" "popfq\n\t"
        "pushfq\n\t" "popq %0\n\t" "cld\n\t" "std\n\t"
        "pushfq\n\t" "popq %1\n\t" "cld"
        : "=&r"(before), "=&r"(after) : : "r11", "cc");
    uint64_t status = CF_FLAG | PF_FLAG | AF_FLAG | ZF_FLAG | SF_FLAG | OF_FLAG;
    TEST_ASSERT(after & DF_FLAG, "std sets DF after cld clears it");
    TEST_ASSERT((before & status) == (after & status),
                "cld/std preserve arithmetic status flags");
}

int main(void) {
    TEST_START("CLC/STC/CMC/CLD/STD instructions");
    test_clc();
    test_stc();
    test_cmc();
    test_cld();
    test_std();
    test_flag_instruction_isolation();
    TEST_END();
}
