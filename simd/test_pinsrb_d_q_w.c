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

#define TEST_PINSRW_POS(imm, input, expected_lane)                           \
    do {                                                                     \
        xmm_t before = { .u16 = {                                            \
            0x1000, 0x1001, 0x1002, 0x1003,                                 \
            0x1004, 0x1005, 0x1006, 0x1007                                  \
        } };                                                                 \
        xmm_t dst = before;                                                  \
        uint32_t val = (input);                                              \
        __asm__ volatile (                                                   \
            "movdqa %0, %%xmm0\n\t"                                         \
            "pinsrw $" #imm ", %1, %%xmm0\n\t"                             \
            "movdqa %%xmm0, %0"                                             \
            : "+m"(dst) : "r"(val) : "xmm0"                               \
        );                                                                   \
        int ok = dst.u16[(expected_lane)] == (uint16_t)val;                  \
        for (int lane = 0; lane < 8; lane++)                                 \
            if (lane != (expected_lane) && dst.u16[lane] != before.u16[lane]) \
                ok = 0;                                                      \
        TEST_ASSERT(ok, "pinsrw $" #imm ": wrong lane or adjacent lane changed"); \
    } while (0)

#define TEST_PINSRB_POS(imm, input, expected_lane)                           \
    do {                                                                     \
        xmm_t before = { .u8 = {                                             \
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,                 \
            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f                  \
        } };                                                                 \
        xmm_t dst = before;                                                  \
        uint32_t val = (input);                                              \
        __asm__ volatile (                                                   \
            "movdqa %0, %%xmm0\n\t"                                         \
            "pinsrb $" #imm ", %1, %%xmm0\n\t"                             \
            "movdqa %%xmm0, %0"                                             \
            : "+m"(dst) : "r"(val) : "xmm0"                               \
        );                                                                   \
        int ok = dst.u8[(expected_lane)] == (uint8_t)val;                    \
        for (int lane = 0; lane < 16; lane++)                                \
            if (lane != (expected_lane) && dst.u8[lane] != before.u8[lane])  \
                ok = 0;                                                      \
        TEST_ASSERT(ok, "pinsrb $" #imm ": wrong lane or adjacent lane changed"); \
    } while (0)

#define TEST_PINSRD_POS(imm, input, expected_lane)                           \
    do {                                                                     \
        xmm_t before = { .u32 = {                                            \
            0x10000000, 0x10000001, 0x10000002, 0x10000003                  \
        } };                                                                 \
        xmm_t dst = before;                                                  \
        uint32_t val = (input);                                              \
        __asm__ volatile (                                                   \
            "movdqa %0, %%xmm0\n\t"                                         \
            "pinsrd $" #imm ", %1, %%xmm0\n\t"                             \
            "movdqa %%xmm0, %0"                                             \
            : "+m"(dst) : "r"(val) : "xmm0"                               \
        );                                                                   \
        int ok = dst.u32[(expected_lane)] == val;                            \
        for (int lane = 0; lane < 4; lane++)                                 \
            if (lane != (expected_lane) && dst.u32[lane] != before.u32[lane]) \
                ok = 0;                                                      \
        TEST_ASSERT(ok, "pinsrd $" #imm ": wrong lane or adjacent lane changed"); \
    } while (0)

#define TEST_PINSRQ_POS(imm, input, expected_lane)                           \
    do {                                                                     \
        xmm_t before = { .u64 = {                                            \
            UINT64_C(0x1000000000000000), UINT64_C(0x1000000000000001)       \
        } };                                                                 \
        xmm_t dst = before;                                                  \
        uint64_t val = (input);                                              \
        __asm__ volatile (                                                   \
            "movdqa %0, %%xmm0\n\t"                                         \
            "pinsrq $" #imm ", %1, %%xmm0\n\t"                             \
            "movdqa %%xmm0, %0"                                             \
            : "+m"(dst) : "r"(val) : "xmm0"                               \
        );                                                                   \
        int other = 1 - (expected_lane);                                     \
        TEST_ASSERT(dst.u64[(expected_lane)] == val &&                       \
                    dst.u64[other] == before.u64[other],                     \
                    "pinsrq $" #imm ": wrong lane or adjacent lane changed"); \
    } while (0)

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
    TEST_PINSRW_POS(0, 0xaaa0, 0);
    TEST_PINSRW_POS(1, 0xaaa1, 1);
    TEST_PINSRW_POS(2, 0xaaa2, 2);
    TEST_PINSRW_POS(3, 0xaaa3, 3);
    TEST_PINSRW_POS(4, 0xaaa4, 4);
    TEST_PINSRW_POS(5, 0xaaa5, 5);
    TEST_PINSRW_POS(6, 0xaaa6, 6);
    TEST_PINSRW_POS(7, 0xaaa7, 7);
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

static void test_pins_all_positions_and_boundaries(void) {
    TEST_PINSRB_POS( 0, 0xdeadbe00,  0);
    TEST_PINSRB_POS( 1, 0xdeadbe01,  1);
    TEST_PINSRB_POS( 2, 0xdeadbe02,  2);
    TEST_PINSRB_POS( 3, 0xdeadbe03,  3);
    TEST_PINSRB_POS( 4, 0xdeadbe04,  4);
    TEST_PINSRB_POS( 5, 0xdeadbe05,  5);
    TEST_PINSRB_POS( 6, 0xdeadbe06,  6);
    TEST_PINSRB_POS( 7, 0xdeadbe07,  7);
    TEST_PINSRB_POS( 8, 0xdeadbe08,  8);
    TEST_PINSRB_POS( 9, 0xdeadbe09,  9);
    TEST_PINSRB_POS(10, 0xdeadbe0a, 10);
    TEST_PINSRB_POS(11, 0xdeadbe0b, 11);
    TEST_PINSRB_POS(12, 0xdeadbe0c, 12);
    TEST_PINSRB_POS(13, 0xdeadbe0d, 13);
    TEST_PINSRB_POS(14, 0xdeadbe0e, 14);
    TEST_PINSRB_POS(15, 0xdeadbe0f, 15);

    TEST_PINSRD_POS(0, UINT32_C(0x80000000), 0);
    TEST_PINSRD_POS(1, UINT32_C(0x7fffffff), 1);
    TEST_PINSRD_POS(2, UINT32_C(0x00000000), 2);
    TEST_PINSRD_POS(3, UINT32_C(0xffffffff), 3);
    TEST_PINSRQ_POS(0, UINT64_C(0x8000000000000000), 0);
    TEST_PINSRQ_POS(1, UINT64_C(0xffffffffffffffff), 1);

    /* High immediate bits are ignored according to the element width. */
    TEST_PINSRB_POS(0x10, UINT32_C(0x123456aa), 0);
    TEST_PINSRB_POS(0xff, UINT32_C(0x123456bb), 15);
    TEST_PINSRW_POS(0x08, UINT32_C(0x1234abcd), 0);
    TEST_PINSRW_POS(0xff, UINT32_C(0x5678beef), 7);
    TEST_PINSRD_POS(0x04, UINT32_C(0x80000000), 0);
    TEST_PINSRD_POS(0xff, UINT32_C(0xffffffff), 3);
    TEST_PINSRQ_POS(0x02, UINT64_C(0x0123456789abcdef), 0);
    TEST_PINSRQ_POS(0xff, UINT64_C(0xfedcba9876543210), 1);
}

int main(void) {
    TEST_START("PINSRB/PINSRD/PINSRQ/PINSRW instructions");
    test_pinsrw_basic();
    test_pinsrw_all_positions();
    test_pinsrb_basic();
    test_pinsrd_basic();
    test_pinsrq_basic();
    test_pinsrd_from_mem();
    test_pins_all_positions_and_boundaries();
    TEST_END();
}
