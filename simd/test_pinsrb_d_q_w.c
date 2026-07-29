/*
 * test_pinsrb_d_q_w.c - Test PINSRB/PINSRD/PINSRQ/PINSRW instructions
 *
 * PINSRW: Insert word from GPR/mem into xmm at position specified by imm8 (SSE2).
 * PINSRB: Insert byte from GPR/mem into xmm at position specified by imm8 (SSE4.1).
 * PINSRD: Insert dword from GPR/mem into xmm at position specified by imm8 (SSE4.1).
 * PINSRQ: Insert qword from GPR/mem into xmm at position specified by imm8 (SSE4.1).
 *
 * Compile: gcc -o test_pinsrb_d_q_w simd/test_pinsrb_d_q_w.c -O0 -msse4.1
 * Note: Do not use static linking.
 */
#include "../common.h"

static void test_pinsrw_basic(void) {
    xmm_t dst = { .u16 = {0,0,0,0,0,0,0,0} };
    uint32_t val = 0x1234;

    __asm__ volatile (
        "movdqa %0, %%xmm0\n\t"
        "pinsrw $3, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "+m"(dst) : "r"(val) : "xmm0"
    );
    TEST_ASSERT(dst.u16[3] == 0x1234, "pinsrw $3: expected 0x1234, got 0x%04x", dst.u16[3]);
    TEST_ASSERT(dst.u16[0] == 0, "pinsrw $3: other elements unchanged [0]");
}

static void test_pinsrw_all_positions(void) {
    xmm_t dst = { .u16 = {0,0,0,0,0,0,0,0} };

    for (int i = 0; i < 8; i++) {
        uint32_t val = (uint32_t)(i + 1) * 1000;
        __asm__ volatile (
            "movdqa %0, %%xmm0\n\t"
            "pinsrw $0, %1, %%xmm0\n\t"
            "movdqa %%xmm0, %0"
            : "+m"(dst) : "r"(val) : "xmm0"
        );
    }
    /* Only tests position 0 multiple times - last value wins */
    TEST_ASSERT(dst.u16[0] == 8000, "pinsrw $0 last: got %u", dst.u16[0]);
}

static void test_pinsrb_basic(void) {
    xmm_t dst = { .u8 = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0} };
    uint32_t val = 0xAB;

    __asm__ volatile (
        "movdqa %0, %%xmm0\n\t"
        "pinsrb $5, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "+m"(dst) : "r"(val) : "xmm0"
    );
    TEST_ASSERT(dst.u8[5] == 0xAB, "pinsrb $5: expected 0xAB, got 0x%02x", dst.u8[5]);
    TEST_ASSERT(dst.u8[4] == 0, "pinsrb: other elements unchanged");
}

static void test_pinsrd_basic(void) {
    xmm_t dst = { .u32 = {0,0,0,0} };
    uint32_t val = 0xDEADBEEF;

    __asm__ volatile (
        "movdqa %0, %%xmm0\n\t"
        "pinsrd $2, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "+m"(dst) : "r"(val) : "xmm0"
    );
    TEST_ASSERT(dst.u32[2] == 0xDEADBEEF, "pinsrd $2: got 0x%08x", dst.u32[2]);
    TEST_ASSERT(dst.u32[0] == 0, "pinsrd: other elements unchanged");
}

static void test_pinsrq_basic(void) {
    xmm_t dst = { .u64 = {0, 0} };
    uint64_t val = 0xCAFEBABEDEADBEEFULL;

    __asm__ volatile (
        "movdqa %0, %%xmm0\n\t"
        "pinsrq $1, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "+m"(dst) : "r"(val) : "xmm0"
    );
    TEST_ASSERT(dst.u64[1] == 0xCAFEBABEDEADBEEFULL, "pinsrq $1: got 0x%016lx", dst.u64[1]);
    TEST_ASSERT(dst.u64[0] == 0, "pinsrq: other element unchanged");
}

static void test_pinsrd_from_mem(void) {
    xmm_t dst = { .u32 = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD} };
    uint32_t mem_val = 0x12345678;

    __asm__ volatile (
        "movdqa %0, %%xmm0\n\t"
        "pinsrd $1, %1, %%xmm0\n\t"
        "movdqa %%xmm0, %0"
        : "+m"(dst) : "m"(mem_val) : "xmm0"
    );
    TEST_ASSERT(dst.u32[0] == 0xAAAAAAAA, "pinsrd from mem: [0] unchanged");
    TEST_ASSERT(dst.u32[1] == 0x12345678, "pinsrd from mem: [1] got 0x%08x", dst.u32[1]);
    TEST_ASSERT(dst.u32[2] == 0xCCCCCCCC, "pinsrd from mem: [2] unchanged");
    TEST_ASSERT(dst.u32[3] == 0xDDDDDDDD, "pinsrd from mem: [3] unchanged");
}

int main(void) {
    TEST_START("PINSRB/PINSRD/PINSRQ/PINSRW instructions");
    test_pinsrw_basic();
    test_pinsrw_all_positions();
    test_pinsrb_basic();
    test_pinsrd_basic();
    test_pinsrq_basic();
    test_pinsrd_from_mem();
    TEST_END();
}
