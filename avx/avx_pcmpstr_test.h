#ifndef X86_64_AVX_PCMPSTR_TEST_H
#define X86_64_AVX_PCMPSTR_TEST_H

#include "../common.h"
#include <limits.h>

typedef struct {
    uint32_t index;
    uint8_t cf, zf, sf, of;
} index_result_t;

typedef struct {
    ymm_t mask;
    uint8_t cf, zf, sf, of;
} mask_result_t;

static void check_flags(uint8_t cf, uint8_t zf, uint8_t sf, uint8_t of,
                        int expected_cf, int expected_zf, int expected_sf,
                        int expected_of, const char *name) {
    TEST_ASSERT(cf == expected_cf, "%s CF expected %d got %u", name, expected_cf, cf);
    TEST_ASSERT(zf == expected_zf, "%s ZF expected %d got %u", name, expected_zf, zf);
    TEST_ASSERT(sf == expected_sf, "%s SF expected %d got %u", name, expected_sf, sf);
    TEST_ASSERT(of == expected_of, "%s OF expected %d got %u", name, expected_of, of);
}

#if TEST_EXPLICIT
#if TEST_MASK_RESULT
#define RUN_STRING(imm, lhs, rhs, lena, lenb, result) do { \
    int32_t _la = (lena), _lb = (lenb); \
    memset(&(result).mask, 0xa5, sizeof((result).mask)); \
    __asm__ volatile("vmovdqu %5, %%ymm0\n\t" \
                     "vmovdqu %6, %%xmm1\n\tvmovdqu %7, %%xmm2\n\t" \
                     "movl %8, %%eax\n\tmovl %9, %%edx\n\t" \
                     TEST_INSN " $" #imm ", %%xmm2, %%xmm1\n\t" \
                     "setc %1\n\tsetz %2\n\tsets %3\n\tseto %4\n\t" \
                     "vmovdqu %%ymm0, %0" \
                     : "=m"((result).mask), "=qm"((result).cf), \
                       "=qm"((result).zf), "=qm"((result).sf), \
                       "=qm"((result).of) \
                     : "m"((result).mask), "m"(lhs), "m"(rhs), \
                       "rm"(_la), "rm"(_lb) \
                     : "xmm0", "xmm1", "xmm2", "eax", "edx", "cc"); \
} while (0)
#else
#define RUN_STRING(imm, lhs, rhs, lena, lenb, result) do { \
    int32_t _la = (lena), _lb = (lenb); \
    __asm__ volatile("vmovdqu %5, %%xmm1\n\tvmovdqu %6, %%xmm2\n\t" \
                     "movl %7, %%eax\n\tmovl %8, %%edx\n\t" \
                     TEST_INSN " $" #imm ", %%xmm2, %%xmm1\n\t" \
                     "movl %%ecx, %0\n\tsetc %1\n\tsetz %2\n\t" \
                     "sets %3\n\tseto %4" \
                     : "=&r"((result).index), "=qm"((result).cf), \
                       "=qm"((result).zf), "=qm"((result).sf), \
                       "=qm"((result).of) \
                     : "m"(lhs), "m"(rhs), "rm"(_la), "rm"(_lb) \
                     : "xmm1", "xmm2", "eax", "edx", "ecx", "cc"); \
} while (0)
#endif
#else
#if TEST_MASK_RESULT
#define RUN_STRING(imm, lhs, rhs, result) do { \
    memset(&(result).mask, 0xa5, sizeof((result).mask)); \
    __asm__ volatile("vmovdqu %5, %%ymm0\n\t" \
                     "vmovdqu %6, %%xmm1\n\tvmovdqu %7, %%xmm2\n\t" \
                     TEST_INSN " $" #imm ", %%xmm2, %%xmm1\n\t" \
                     "setc %1\n\tsetz %2\n\tsets %3\n\tseto %4\n\t" \
                     "vmovdqu %%ymm0, %0" \
                     : "=m"((result).mask), "=qm"((result).cf), \
                       "=qm"((result).zf), "=qm"((result).sf), \
                       "=qm"((result).of) \
                     : "m"((result).mask), "m"(lhs), "m"(rhs) \
                     : "xmm0", "xmm1", "xmm2", "cc"); \
} while (0)
#else
#define RUN_STRING(imm, lhs, rhs, result) do { \
    __asm__ volatile("vmovdqu %5, %%xmm1\n\tvmovdqu %6, %%xmm2\n\t" \
                     TEST_INSN " $" #imm ", %%xmm2, %%xmm1\n\t" \
                     "movl %%ecx, %0\n\tsetc %1\n\tsetz %2\n\t" \
                     "sets %3\n\tseto %4" \
                     : "=&r"((result).index), "=qm"((result).cf), \
                       "=qm"((result).zf), "=qm"((result).sf), \
                       "=qm"((result).of) \
                     : "m"(lhs), "m"(rhs) \
                     : "xmm1", "xmm2", "ecx", "cc"); \
} while (0)
#endif
#endif

#if TEST_MASK_RESULT
static void check_upper_zero(const mask_result_t *r, const char *name) {
    for (int i = 16; i < 32; i++)
        TEST_ASSERT(r->mask.u8[i] == 0, "%s VEX upper byte %d", name, i);
}

static void test_masks(void) {
    xmm_t a, b;
    mask_result_t r;
    memset(&a, 'X', sizeof(a));
    memset(&b, 'X', sizeof(b));
    memcpy(a.u8, "ABCD", 4);
    memcpy(b.u8, "ABCD", 4);
#if TEST_EXPLICIT
    RUN_STRING(0x08, a, b, 16, 4, r);
#else
    b.u8[4] = 0;
    RUN_STRING(0x08, a, b, r);
#endif
    TEST_ASSERT((r.mask.u32[0] & 0xffffU) == 0x000fU,
                TEST_NAME " positive mask %#x", r.mask.u32[0] & 0xffffU);
    check_flags(r.cf, r.zf, r.sf, r.of, 1, 1, TEST_EXPLICIT ? 0 : 0, 1,
                "positive polarity");
    check_upper_zero(&r, "positive polarity");

#if TEST_EXPLICIT
    RUN_STRING(0x18, a, b, 16, 4, r);
#else
    RUN_STRING(0x18, a, b, r);
#endif
    TEST_ASSERT((r.mask.u32[0] & 0xffffU) == 0xfff0U,
                TEST_NAME " negative mask %#x", r.mask.u32[0] & 0xffffU);
#if TEST_EXPLICIT
    RUN_STRING(0x28, a, b, 16, 4, r);
#else
    RUN_STRING(0x28, a, b, r);
#endif
    TEST_ASSERT((r.mask.u32[0] & 0xffffU) == 0x000fU,
                TEST_NAME " masked-positive mask");
#if TEST_EXPLICIT
    RUN_STRING(0x38, a, b, 16, 4, r);
#else
    RUN_STRING(0x38, a, b, r);
#endif
    TEST_ASSERT((r.mask.u32[0] & 0xffffU) == 0,
                TEST_NAME " masked-negative mask");

    memset(&a, 'K', sizeof(a));
    b = a;
    b.u8[1] = 'x';
    b.u8[14] = 'y';
#if TEST_EXPLICIT
    RUN_STRING(0x18, a, b, 16, 16, r);
#else
    RUN_STRING(0x18, a, b, r);
#endif
    TEST_ASSERT((r.mask.u32[0] & 0xffffU) == 0x4002U,
                TEST_NAME " endpoint bit mask %#x", r.mask.u32[0] & 0xffffU);
    check_flags(r.cf, r.zf, r.sf, r.of, 1, 0, 0, 0, "bit mask flags");
#if TEST_EXPLICIT
    RUN_STRING(0x58, a, b, 16, 16, r);
#else
    RUN_STRING(0x58, a, b, r);
#endif
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(r.mask.u8[i] == ((i == 1 || i == 14) ? 0xff : 0),
                    TEST_NAME " unit mask lane %d", i);
}
#else
static void test_indices(void) {
    xmm_t a, b;
    index_result_t r;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
#if TEST_EXPLICIT
    RUN_STRING(0x08, a, b, 0, 0, r);
#else
    RUN_STRING(0x08, a, b, r);
#endif
    TEST_ASSERT(r.index == 0, TEST_NAME " empty index %u", r.index);
    check_flags(r.cf, r.zf, r.sf, r.of, 1, 1, 1, 1, "empty strings");

    memset(&a, 'K', sizeof(a));
    b = a;
    b.u8[0] = 'x';
    b.u8[15] = 'y';
#if TEST_EXPLICIT
    RUN_STRING(0x18, a, b, 16, 16, r);
#else
    RUN_STRING(0x18, a, b, r);
#endif
    TEST_ASSERT(r.index == 0, TEST_NAME " lowest index %u", r.index);
    check_flags(r.cf, r.zf, r.sf, r.of, 1, 0, 0, 1, "lowest index");
#if TEST_EXPLICIT
    RUN_STRING(0x58, a, b, 16, 16, r);
#else
    RUN_STRING(0x58, a, b, r);
#endif
    TEST_ASSERT(r.index == 15, TEST_NAME " highest index %u", r.index);

#if TEST_EXPLICIT
    b = a;
    b.u8[15] = 'X';
    RUN_STRING(0x18, a, b, 15, 15, r);
    TEST_ASSERT(r.index == 16, TEST_NAME " length 15 excludes final lane");
    RUN_STRING(0x18, a, b, 16, 16, r);
    TEST_ASSERT(r.index == 15, TEST_NAME " length 16 includes final lane");
    RUN_STRING(0x18, a, b, 17, 17, r);
    TEST_ASSERT(r.index == 15, TEST_NAME " length 17 saturates");
    b = a;
    b.u8[3] = 'X';
    RUN_STRING(0x18, a, b, -4, -4, r);
    TEST_ASSERT(r.index == 3, TEST_NAME " negative lengths use absolute value");
#else
    memset(&a, 'Q', sizeof(a));
    b = a;
    b.u8[15] = 'X';
    RUN_STRING(0x18, a, b, r);
    TEST_ASSERT(r.index == 15, TEST_NAME " full-width final mismatch");
#endif
}
#endif

int main(void) {
    TEST_START(TEST_NAME);
#if TEST_MASK_RESULT
    test_masks();
#else
    test_indices();
#endif
    TEST_END();
}

#endif
