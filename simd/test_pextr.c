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

/*
 * Register destinations for PEXTRB and PEXTRW are zero-extended to 32 bits.
 * Seed the output with all ones and use a read/write constraint so a partial
 * register write cannot accidentally pass these tests.
 */
#define TEST_PEXTRB_REG(src, imm, expected)                                  \
    do {                                                                     \
        uint32_t out = UINT32_MAX;                                            \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrb $" #imm ", %%xmm0, %0\n\t"                            \
            : "+r"(out) : "m"(src) : "xmm0"                              \
        );                                                                   \
        TEST_ASSERT(out == (uint32_t)(expected),                             \
                    "pextrb $" #imm " to reg: expected 0x%02x, got 0x%08x", \
                    (unsigned)(expected), out);                              \
    } while (0)

#define TEST_PEXTRW_REG(src, imm, expected)                                  \
    do {                                                                     \
        uint32_t out = UINT32_MAX;                                            \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrw $" #imm ", %%xmm0, %0\n\t"                            \
            : "+r"(out) : "m"(src) : "xmm0"                              \
        );                                                                   \
        TEST_ASSERT(out == (uint32_t)(expected),                             \
                    "pextrw $" #imm " to reg: expected 0x%04x, got 0x%08x", \
                    (unsigned)(expected), out);                              \
    } while (0)

#define TEST_PEXTRD_REG(src, imm, expected)                                  \
    do {                                                                     \
        uint32_t out = UINT32_MAX;                                            \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrd $" #imm ", %%xmm0, %0\n\t"                            \
            : "+r"(out) : "m"(src) : "xmm0"                              \
        );                                                                   \
        TEST_ASSERT(out == (uint32_t)(expected),                             \
                    "pextrd $" #imm " to reg: expected 0x%08x, got 0x%08x", \
                    (uint32_t)(expected), out);                              \
    } while (0)

#define TEST_PEXTRQ_REG(src, imm, expected)                                  \
    do {                                                                     \
        uint64_t out = UINT64_MAX;                                            \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrq $" #imm ", %%xmm0, %0\n\t"                            \
            : "+r"(out) : "m"(src) : "xmm0"                              \
        );                                                                   \
        TEST_ASSERT(out == (uint64_t)(expected),                             \
                    "pextrq $" #imm " to reg: expected 0x%016" PRIx64       \
                    ", got 0x%016" PRIx64,                                  \
                    (uint64_t)(expected), out);                              \
    } while (0)

/* Guard values verify that the memory form stores exactly one element. */
#define TEST_PEXTRB_MEM(src, imm, expected)                                  \
    do {                                                                     \
        struct { uint8_t before, value, after; } mem =                       \
            { 0xa5, 0xcc, 0x5a };                                            \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrb $" #imm ", %%xmm0, %0\n\t"                            \
            : "=m"(mem.value) : "m"(src) : "xmm0", "memory"               \
        );                                                                   \
        TEST_ASSERT(mem.value == (uint8_t)(expected),                        \
                    "pextrb $" #imm " to mem: expected 0x%02x, got 0x%02x", \
                    (unsigned)(expected), mem.value);                        \
        TEST_ASSERT(mem.before == 0xa5 && mem.after == 0x5a,                 \
                    "pextrb $" #imm " to mem overwrote adjacent bytes");    \
    } while (0)

#define TEST_PEXTRW_MEM(src, imm, expected)                                  \
    do {                                                                     \
        struct { uint16_t before, value, after; } mem =                      \
            { 0xa55a, 0xcccc, 0x5aa5 };                                      \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrw $" #imm ", %%xmm0, %0\n\t"                            \
            : "=m"(mem.value) : "m"(src) : "xmm0", "memory"               \
        );                                                                   \
        TEST_ASSERT(mem.value == (uint16_t)(expected),                       \
                    "pextrw $" #imm " to mem: expected 0x%04x, got 0x%04x", \
                    (unsigned)(expected), mem.value);                        \
        TEST_ASSERT(mem.before == 0xa55a && mem.after == 0x5aa5,             \
                    "pextrw $" #imm " to mem overwrote adjacent words");    \
    } while (0)

#define TEST_PEXTRD_MEM(src, imm, expected)                                  \
    do {                                                                     \
        struct { uint32_t before, value, after; } mem =                      \
            { 0xa55aa55a, 0xcccccccc, 0x5aa55aa5 };                          \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrd $" #imm ", %%xmm0, %0\n\t"                            \
            : "=m"(mem.value) : "m"(src) : "xmm0", "memory"               \
        );                                                                   \
        TEST_ASSERT(mem.value == (uint32_t)(expected),                       \
                    "pextrd $" #imm " to mem: expected 0x%08x, got 0x%08x", \
                    (uint32_t)(expected), mem.value);                        \
        TEST_ASSERT(mem.before == 0xa55aa55a && mem.after == 0x5aa55aa5,     \
                    "pextrd $" #imm " to mem overwrote adjacent dwords");   \
    } while (0)

#define TEST_PEXTRQ_MEM(src, imm, expected)                                  \
    do {                                                                     \
        struct { uint64_t before, value, after; } mem = {                    \
            UINT64_C(0xa55aa55aa55aa55a), UINT64_C(0xcccccccccccccccc),      \
            UINT64_C(0x5aa55aa55aa55aa5)                                    \
        };                                                                   \
        __asm__ volatile (                                                   \
            "movdqa %1, %%xmm0\n\t"                                         \
            "pextrq $" #imm ", %%xmm0, %0\n\t"                            \
            : "=m"(mem.value) : "m"(src) : "xmm0", "memory"               \
        );                                                                   \
        TEST_ASSERT(mem.value == (uint64_t)(expected),                       \
                    "pextrq $" #imm " to mem: expected 0x%016" PRIx64       \
                    ", got 0x%016" PRIx64,                                  \
                    (uint64_t)(expected), mem.value);                        \
        TEST_ASSERT(mem.before == UINT64_C(0xa55aa55aa55aa55a) &&            \
                    mem.after == UINT64_C(0x5aa55aa55aa55aa5),               \
                    "pextrq $" #imm " to mem overwrote adjacent qwords");   \
    } while (0)

static void test_pextrb_all_lanes_to_reg(void) {
    xmm_t src = { .u8 = {
        0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff, 0x55, 0xaa,
        0x10, 0x20, 0x40, 0x81, 0xc3, 0x5a, 0xa5, 0xf0
    } };

    TEST_PEXTRB_REG(src,  0, 0x00);
    TEST_PEXTRB_REG(src,  1, 0x01);
    TEST_PEXTRB_REG(src,  2, 0x7f);
    TEST_PEXTRB_REG(src,  3, 0x80);
    TEST_PEXTRB_REG(src,  4, 0xfe);
    TEST_PEXTRB_REG(src,  5, 0xff);
    TEST_PEXTRB_REG(src,  6, 0x55);
    TEST_PEXTRB_REG(src,  7, 0xaa);
    TEST_PEXTRB_REG(src,  8, 0x10);
    TEST_PEXTRB_REG(src,  9, 0x20);
    TEST_PEXTRB_REG(src, 10, 0x40);
    TEST_PEXTRB_REG(src, 11, 0x81);
    TEST_PEXTRB_REG(src, 12, 0xc3);
    TEST_PEXTRB_REG(src, 13, 0x5a);
    TEST_PEXTRB_REG(src, 14, 0xa5);
    TEST_PEXTRB_REG(src, 15, 0xf0);
}

static void test_pextrb_boundaries(void) {
    xmm_t src = { .u8 = {
        0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff, 0x55, 0xaa,
        0x10, 0x20, 0x40, 0x81, 0xc3, 0x5a, 0xa5, 0xf0
    } };

    /* Only imm8[3:0] selects the byte: 0x10 -> 0 and 0xff -> 15. */
    TEST_PEXTRB_REG(src, 0x10, 0x00);
    TEST_PEXTRB_REG(src, 0xff, 0xf0);
    TEST_PEXTRB_MEM(src, 0x00, 0x00);
    TEST_PEXTRB_MEM(src, 0xff, 0xf0);
}

static void test_pextrw_all_lanes_to_reg(void) {
    xmm_t src = { .u16 = {
        0x0000, 0x0001, 0x7fff, 0x8000,
        0xfffe, 0xffff, 0x55aa, 0xaa55
    } };

    TEST_PEXTRW_REG(src, 0, 0x0000);
    TEST_PEXTRW_REG(src, 1, 0x0001);
    TEST_PEXTRW_REG(src, 2, 0x7fff);
    TEST_PEXTRW_REG(src, 3, 0x8000);
    TEST_PEXTRW_REG(src, 4, 0xfffe);
    TEST_PEXTRW_REG(src, 5, 0xffff);
    TEST_PEXTRW_REG(src, 6, 0x55aa);
    TEST_PEXTRW_REG(src, 7, 0xaa55);
}

static void test_pextrw_boundaries(void) {
    xmm_t src = { .u16 = {
        0x0000, 0x0001, 0x7fff, 0x8000,
        0xfffe, 0xffff, 0x55aa, 0xaa55
    } };

    /* Only imm8[2:0] selects the word: 0x08 -> 0 and 0xff -> 7. */
    TEST_PEXTRW_REG(src, 0x08, 0x0000);
    TEST_PEXTRW_REG(src, 0xff, 0xaa55);
    TEST_PEXTRW_MEM(src, 0x00, 0x0000);
    TEST_PEXTRW_MEM(src, 0xff, 0xaa55);
}

static void test_pextrd_all_lanes_to_reg(void) {
    xmm_t src = { .u32 = {
        0x00000000, 0x7fffffff, 0x80000000, 0xffffffff
    } };

    TEST_PEXTRD_REG(src, 0, 0x00000000);
    TEST_PEXTRD_REG(src, 1, 0x7fffffff);
    TEST_PEXTRD_REG(src, 2, 0x80000000);
    TEST_PEXTRD_REG(src, 3, 0xffffffff);
}

static void test_pextrd_boundaries(void) {
    xmm_t src = { .u32 = {
        0x00000000, 0x7fffffff, 0x80000000, 0xffffffff
    } };

    /* Only imm8[1:0] selects the dword: 0x04 -> 0 and 0xff -> 3. */
    TEST_PEXTRD_REG(src, 0x04, 0x00000000);
    TEST_PEXTRD_REG(src, 0xff, 0xffffffff);
    TEST_PEXTRD_MEM(src, 0x00, 0x00000000);
    TEST_PEXTRD_MEM(src, 0xff, 0xffffffff);
}

static void test_pextrq_all_lanes_to_reg(void) {
    xmm_t src = { .u64 = {
        UINT64_C(0x0000000000000000), UINT64_C(0xffffffffffffffff)
    } };

    TEST_PEXTRQ_REG(src, 0, UINT64_C(0x0000000000000000));
    TEST_PEXTRQ_REG(src, 1, UINT64_C(0xffffffffffffffff));
}

static void test_pextrq_boundaries(void) {
    xmm_t src = { .u64 = {
        UINT64_C(0x1122334455667788), UINT64_C(0x99aabbccddeeff00)
    } };

    /* Only imm8[0] selects the qword: 0x02 -> 0 and 0xff -> 1. */
    TEST_PEXTRQ_REG(src, 0x02, UINT64_C(0x1122334455667788));
    TEST_PEXTRQ_REG(src, 0xff, UINT64_C(0x99aabbccddeeff00));
    TEST_PEXTRQ_MEM(src, 0x00, UINT64_C(0x1122334455667788));
    TEST_PEXTRQ_MEM(src, 0xff, UINT64_C(0x99aabbccddeeff00));
}

int main(void) {
    TEST_START("PEXTRB/PEXTRW/PEXTRD/PEXTRQ");
    test_pextrb_all_lanes_to_reg();
    test_pextrb_boundaries();
    test_pextrw_all_lanes_to_reg();
    test_pextrw_boundaries();
    test_pextrd_all_lanes_to_reg();
    test_pextrd_boundaries();
    test_pextrq_all_lanes_to_reg();
    test_pextrq_boundaries();
    TEST_END();
}
