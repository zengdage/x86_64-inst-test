/*
 * test_pdep_pext.c - Test x86-64 PDEP/PEXT instructions (BMI2)
 *
 * PDEP: Parallel bit deposit. Deposits contiguous low bits of src into
 *       positions selected by mask.
 * PEXT: Parallel bit extract. Extracts bits from src at positions selected
 *       by mask into contiguous low bits.
 * Neither affects flags.
 *
 * Compile: gcc -o test_pdep_pext integer/test_pdep_pext.c -O0 -mbmi2
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pdep_basic(void) {
    uint64_t result;

    /* Deposit bits into every other position */
    /* src = 0b1010 = 0xA, mask = 0b01010101 = 0x55 */
    /* Deposits: bit0 of src -> bit0, bit1 -> bit2, bit2 -> bit4, bit3 -> bit6 */
    /* 0xA = 1010 => bit0=0->pos0, bit1=1->pos2, bit2=0->pos4, bit3=1->pos6 */
    /* result = 0b01000100 = 0x44 */
    __asm__ volatile (
        "movq $0x0A, %%rax\n\t"
        "movq $0x55, %%rbx\n\t"
        "pdepq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 0x44, "pdepq 0xA mask 0x55: expected 0x44, got 0x%lx", result);
}

static void test_pdep_all_ones_mask(void) {
    uint64_t result;

    /* Mask = all ones => result = src (identity) */
    __asm__ volatile (
        "movq $0x12345678, %%rax\n\t"
        "movq $-1, %%rbx\n\t"
        "pdepq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 0x12345678, "pdepq all-ones mask: identity");
}

static void test_pdep_zero_mask(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0xFFFFFFFF, %%rax\n\t"
        "xorq %%rbx, %%rbx\n\t"
        "pdepq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 0, "pdepq zero mask: expected 0");
}

static void test_pext_basic(void) {
    uint64_t result;

    /* Extract every other bit */
    /* src = 0b11001010 = 0xCA, mask = 0b01010101 = 0x55 */
    /* Extract: bit0=0, bit2=0, bit4=0, bit6=1 => 0b1000 = 8 */
    __asm__ volatile (
        "movq $0xCA, %%rax\n\t"
        "movq $0x55, %%rbx\n\t"
        "pextq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 8, "pextq 0xCA mask 0x55: expected 8, got 0x%lx", result);
}

static void test_pext_all_ones_mask(void) {
    uint64_t result;

    __asm__ volatile (
        "movq $0xABCDEF01, %%rax\n\t"
        "movq $-1, %%rbx\n\t"
        "pextq %%rbx, %%rax, %%rcx\n\t"
        "movq %%rcx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    TEST_ASSERT(result == 0xABCDEF01, "pextq all-ones mask: identity");
}

static void test_pdep_pext_inverse(void) {
    uint64_t src = 0x12345678;
    uint64_t mask = 0xFF00FF00FF00FF00UL;
    uint64_t deposited, extracted;

    /* PDEP then PEXT with same mask should return original (for bits that fit) */
    __asm__ volatile (
        "pdepq %2, %3, %0\n\t"
        "pextq %2, %0, %1"
        : "=&r"(deposited), "=r"(extracted)
        : "r"(mask), "r"(src)
    );
    /* Only the low N bits of src matter (N = popcount of mask = 32) */
    TEST_ASSERT(extracted == src, "pdep then pext: roundtrip, got 0x%lx", extracted);
}

static void test_pdep_pext_32bit(void) {
    uint32_t result;

    __asm__ volatile (
        "movl $0xFF, %%eax\n\t"
        "movl $0x0F0F0F0F, %%ebx\n\t"
        "pdepl %%ebx, %%eax, %%ecx\n\t"
        "movl %%ecx, %0"
        : "=r"(result)
        :
        : "rax", "rbx", "rcx"
    );
    /* 0xFF = 8 bits, mask has 16 set bits in groups of 4 */
    /* Low 8 bits go into first 8 mask positions */
    /* mask positions: 0-3, 8-11, 16-19, 24-27 */
    /* bits 0-3 of src (0xF) -> positions 0-3 */
    /* bits 4-7 of src (0xF) -> positions 8-11 */
    TEST_ASSERT(result == 0x00000F0F, "pdepl 0xFF mask 0x0F0F0F0F: expected 0x0F0F, got 0x%x", result);
}

int main(void) {
    TEST_START("PDEP/PEXT instructions (BMI2)");
    test_pdep_basic();
    test_pdep_all_ones_mask();
    test_pdep_zero_mask();
    test_pext_basic();
    test_pext_all_ones_mask();
    test_pdep_pext_inverse();
    test_pdep_pext_32bit();
    TEST_END();
}
