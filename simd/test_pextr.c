/*
 * test_pextr.c - Test SSE4.1 PEXTRB/PEXTRW/PEXTRD/PEXTRQ instructions
 *
 * PEXTRB: Extract byte from xmm at position specified by imm8 to GPR/mem (SSE4.1).
 * PEXTRW: Extract word from xmm at position specified by imm8 to GPR/mem (SSE4.1).
 *          (Note: SSE2 has PEXTRW reg-only; SSE4.1 adds mem form and is what we test here.)
 * PEXTRD: Extract dword from xmm at position specified by imm8 to GPR/mem (SSE4.1).
 * PEXTRQ: Extract qword from xmm at position specified by imm8 to GPR/mem (SSE4.1).
 *
 * Compile: gcc -o test_pextr simd/test_pextr.c -O2 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pextrb_to_reg(void) {
    xmm_t src = { .u8 = {0,1,2,3,4,5,6,7, 8,9,10,11,12,13,14,15} };
    uint32_t out = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrb $5, %%xmm0, %0\n\t"
        : "=r"(out) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(out == 5u, "pextrb $5: expected 5, got %u", out);
}

static void test_pextrb_to_mem(void) {
    xmm_t src = { .u8 = {0,1,2,3,4,5,6,7, 8,9,10,11,12,13,14,15} };
    uint8_t mem = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrb $10, %%xmm0, %0\n\t"
        : "=m"(mem) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(mem == 10u, "pextrb $10 to mem: expected 10, got %u", mem);
}

static void test_pextrw_to_reg(void) {
    xmm_t src = { .u16 = {100, 200, 300, 400, 500, 600, 700, 800} };
    uint32_t out = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrw $3, %%xmm0, %0\n\t"
        : "=r"(out) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(out == 400u, "pextrw $3: expected 400, got %u", out);
}

static void test_pextrw_to_mem(void) {
    xmm_t src = { .u16 = {100, 200, 300, 400, 500, 600, 700, 800} };
    uint16_t mem = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrw $6, %%xmm0, %0\n\t"
        : "=m"(mem) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(mem == 700u, "pextrw $6 to mem: expected 700, got %u", mem);
}

static void test_pextrd_to_reg(void) {
    xmm_t src = { .u32 = {0xDEADBEEF, 0x12345678, 0xCAFEBABE, 0xFEEDFACE} };
    uint32_t out = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrd $2, %%xmm0, %0\n\t"
        : "=r"(out) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(out == 0xCAFEBABEu, "pextrd $2: expected 0xCAFEBABE, got 0x%x", out);
}

static void test_pextrd_to_mem(void) {
    xmm_t src = { .u32 = {0xDEADBEEF, 0x12345678, 0xCAFEBABE, 0xFEEDFACE} };
    uint32_t mem = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrd $1, %%xmm0, %0\n\t"
        : "=m"(mem) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(mem == 0x12345678u, "pextrd $1 to mem: expected 0x12345678, got 0x%x", mem);
}

static void test_pextrq_to_reg(void) {
    xmm_t src = { .u64 = {0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL} };
    uint64_t out = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrq $1, %%xmm0, %0\n\t"
        : "=r"(out) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(out == 0x99AABBCCDDEEFF00ULL, "pextrq $1: got 0x%016lx", out);
}

static void test_pextrq_to_mem(void) {
    xmm_t src = { .u64 = {0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL} };
    uint64_t mem = 0;
    __asm__ volatile (
        "movdqa %1, %%xmm0\n\t"
        "pextrq $0, %%xmm0, %0\n\t"
        : "=m"(mem) : "m"(src) : "xmm0"
    );
    TEST_ASSERT(mem == 0x1122334455667788ULL, "pextrq $0 to mem: got 0x%016lx", mem);
}

int main(void) {
    TEST_START("PEXTRB/PEXTRW/PEXTRD/PEXTRQ");
    test_pextrb_to_reg();
    test_pextrb_to_mem();
    test_pextrw_to_reg();
    test_pextrw_to_mem();
    test_pextrd_to_reg();
    test_pextrd_to_mem();
    test_pextrq_to_reg();
    test_pextrq_to_mem();
    TEST_END();
}
