/*
 * test_pcmpestri.c - Test PCMPESTRI/PCMPESTRM instructions (SSE4.2)
 *
 * PCMPESTRI: Packed compare explicit-length strings, return index in ECX.
 * PCMPESTRM: Packed compare explicit-length strings, return mask in XMM0.
 * String lengths are explicitly given in EAX (for xmm1) and EDX (for xmm2).
 *
 * Compile: gcc -o test_pcmpestri simd/test_pcmpestri.c -O0 -msse4.2
 * Note: Do not use static linking.
 */
#include "../common.h"
#include <limits.h>

typedef struct {
    uint32_t index;
    uint8_t cf, zf, sf, of;
} pcmp_e_result;

typedef struct {
    xmm_t mask;
    uint8_t cf, zf, sf, of;
} pcmp_em_result;

#define RUN_PCMPESTRI(imm, lhs, rhs, lena, lenb, result) do { \
    int32_t _la = (lena), _lb = (lenb); \
    __asm__ volatile ( \
        "movdqa %5, %%xmm1\n\t" \
        "movdqa %6, %%xmm2\n\t" \
        "movl %7, %%eax\n\t" \
        "movl %8, %%edx\n\t" \
        "pcmpestri $" #imm ", %%xmm2, %%xmm1\n\t" \
        "movl %%ecx, %0\n\t" \
        "setc %1\n\t" \
        "setz %2\n\t" \
        "sets %3\n\t" \
        "seto %4" \
        : "=&r"((result).index), "=qm"((result).cf), "=qm"((result).zf), \
          "=qm"((result).sf), "=qm"((result).of) \
        : "m"(lhs), "m"(rhs), "rm"(_la), "rm"(_lb) \
        : "xmm1", "xmm2", "eax", "edx", "ecx", "cc"); \
} while (0)

#define RUN_PCMPESTRM(imm, lhs, rhs, lena, lenb, result) do { \
    int32_t _la = (lena), _lb = (lenb); \
    __asm__ volatile ( \
        "movdqa %5, %%xmm1\n\t" \
        "movdqa %6, %%xmm2\n\t" \
        "movl %7, %%eax\n\t" \
        "movl %8, %%edx\n\t" \
        "pcmpestrm $" #imm ", %%xmm2, %%xmm1\n\t" \
        "movdqa %%xmm0, %0\n\t" \
        "setc %1\n\t" \
        "setz %2\n\t" \
        "sets %3\n\t" \
        "seto %4" \
        : "=m"((result).mask), "=qm"((result).cf), "=qm"((result).zf), \
          "=qm"((result).sf), "=qm"((result).of) \
        : "m"(lhs), "m"(rhs), "rm"(_la), "rm"(_lb) \
        : "xmm0", "xmm1", "xmm2", "eax", "edx", "cc"); \
} while (0)

static void assert_eflags(pcmp_e_result r, int cf, int zf, int sf, int of, const char *name) {
    TEST_ASSERT(r.cf == cf, "%s CF expected %d got %u", name, cf, r.cf);
    TEST_ASSERT(r.zf == zf, "%s ZF expected %d got %u", name, zf, r.zf);
    TEST_ASSERT(r.sf == sf, "%s SF expected %d got %u", name, sf, r.sf);
    TEST_ASSERT(r.of == of, "%s OF expected %d got %u", name, of, r.of);
}

static void test_pcmpestri_equal_each(void) {
    xmm_t a, b;
    memset(&a, 0xFF, sizeof(a));
    memset(&b, 0xFF, sizeof(b));
    memcpy(a.u8, "ABCDEF", 6);
    memcpy(b.u8, "ABCXEF", 6);

    uint32_t idx;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "movl $6, %%eax\n\t"   /* length of a */
        "movl $6, %%edx\n\t"   /* length of b */
        "pcmpestri $0x18, %%xmm2, %%xmm1\n\t"  /* equal each, negative polarity */
        "movl %%ecx, %0"
        : "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "eax", "edx", "ecx", "cc"
    );
    /* Mismatch at index 3 */
    TEST_ASSERT(idx == 3, "pcmpestri equal each mismatch: expected 3, got %u", idx);
}

static void test_pcmpestri_all_match(void) {
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "HELLO", 5);
    memcpy(b.u8, "HELLO", 5);

    uint32_t idx;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "movl $5, %%eax\n\t"
        "movl $5, %%edx\n\t"
        "pcmpestri $0x18, %%xmm2, %%xmm1\n\t"
        "movl %%ecx, %0"
        : "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "eax", "edx", "ecx", "cc"
    );
    /* All match, negative polarity => no bits set => idx = 16 */
    TEST_ASSERT(idx == 16, "pcmpestri all match: expected 16, got %u", idx);
}

static void test_pcmpestri_partial_length(void) {
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "AB", 2);
    memcpy(b.u8, "ABCDEF", 6);

    uint32_t idx;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "movl $2, %%eax\n\t"   /* only compare 2 bytes of a */
        "movl $6, %%edx\n\t"
        "pcmpestri $0x08, %%xmm2, %%xmm1\n\t"  /* equal each, positive polarity */
        "movl %%ecx, %0"
        : "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "eax", "edx", "ecx", "cc"
    );
    /* First 2 bytes match */
    TEST_ASSERT(idx == 0, "pcmpestri partial len first match at 0: got %u", idx);
}

static void test_pcmpestrm_equal_each(void) {
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "ABCD", 4);
    memcpy(b.u8, "AXCX", 4);

    xmm_t result;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "movl $4, %%eax\n\t"
        "movl $4, %%edx\n\t"
        "pcmpestrm $0x08, %%xmm2, %%xmm1\n\t"  /* equal each, positive, bit mask */
        "movdqa %%xmm0, %0"
        : "=m"(result) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2", "eax", "edx", "cc"
    );
    TEST_ASSERT((result.u32[0] & 0xF) == 0x05,
        "pcmpestrm equal each: expected 0x05, got 0x%x", result.u32[0] & 0xF);
}

static void test_pcmpestri_flags(void) {
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "ABC", 3);
    memcpy(b.u8, "ABC", 3);

    uint64_t flags;
    uint32_t idx;
    __asm__ volatile (
        "movdqa %2, %%xmm1\n\t"
        "movdqa %3, %%xmm2\n\t"
        "movl $3, %%eax\n\t"
        "movl $3, %%edx\n\t"
        "pcmpestri $0x08, %%xmm2, %%xmm1\n\t"
        "movl %%ecx, %1\n\t"
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags), "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "eax", "edx", "ecx", "cc"
    );
    /* CF=1 if result is not zero (there are matches) */
    TEST_ASSERT(flags & CF_FLAG, "pcmpestri flags: CF should be set (result not zero)");
}

static void test_pcmpestri_length_boundaries(void) {
    xmm_t a, b;
    pcmp_e_result r;
    memset(&a, 'A', sizeof(a));
    memset(&b, 'A', sizeof(b));

    RUN_PCMPESTRI(0x08, a, b, 0, 0, r);
    TEST_ASSERT(r.index == 0, "pcmpestri both length zero index=%u", r.index);
    assert_eflags(r, 1, 1, 1, 1, "length 0/0");

    RUN_PCMPESTRI(0x08, a, b, 0, 16, r);
    TEST_ASSERT(r.index == 16, "pcmpestri lhs length zero index=%u", r.index);
    assert_eflags(r, 0, 0, 1, 0, "length 0/16");

    b.u8[3] = 'X';
    RUN_PCMPESTRI(0x18, a, b, -4, -4, r);
    TEST_ASSERT(r.index == 3, "pcmpestri negative lengths use absolute value: %u", r.index);
    assert_eflags(r, 1, 1, 1, 0, "negative lengths");

    memset(&b, 'A', sizeof(b));
    b.u8[15] = 'X';
    RUN_PCMPESTRI(0x18, a, b, 15, 15, r);
    TEST_ASSERT(r.index == 16, "pcmpestri length 15 excludes lane 15: %u", r.index);
    assert_eflags(r, 0, 1, 1, 0, "length 15/15");
    RUN_PCMPESTRI(0x18, a, b, 16, 16, r);
    TEST_ASSERT(r.index == 15, "pcmpestri length 16 includes lane 15: %u", r.index);
    assert_eflags(r, 1, 0, 0, 0, "length 16/16");
    RUN_PCMPESTRI(0x18, a, b, 17, 17, r);
    TEST_ASSERT(r.index == 15, "pcmpestri length 17 saturates to 16: %u", r.index);
    assert_eflags(r, 1, 0, 0, 0, "length 17/17");
    RUN_PCMPESTRI(0x18, a, b, INT_MIN, INT_MIN, r);
    TEST_ASSERT(r.index == 15, "pcmpestri INT_MIN length saturates to 16: %u", r.index);
    assert_eflags(r, 1, 0, 0, 0, "length INT_MIN");
}

static void test_pcmpestri_controls(void) {
    xmm_t a, b;
    pcmp_e_result r;
    pcmp_em_result mr;

    memset(&a, 'X', sizeof(a));
    memset(&b, 'X', sizeof(b));
    memcpy(a.u8, "ABCD", 4);
    memcpy(b.u8, "ABCD", 4);
    RUN_PCMPESTRM(0x08, a, b, 16, 4, mr);
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x000fu, "pcmpestrm positive mask=0x%04x", mr.mask.u32[0] & 0xffffu);
    RUN_PCMPESTRM(0x18, a, b, 16, 4, mr);
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0xfff0u, "pcmpestrm negative mask=0x%04x", mr.mask.u32[0] & 0xffffu);
    RUN_PCMPESTRM(0x28, a, b, 16, 4, mr);
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x000fu, "pcmpestrm masked-positive mask=0x%04x", mr.mask.u32[0] & 0xffffu);
    RUN_PCMPESTRM(0x38, a, b, 16, 4, mr);
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x0000u, "pcmpestrm masked-negative mask=0x%04x", mr.mask.u32[0] & 0xffffu);

    memset(&a, 'K', sizeof(a));
    b = a;
    b.u8[0] = 'x';
    b.u8[15] = 'y';
    RUN_PCMPESTRI(0x18, a, b, 16, 16, r);
    TEST_ASSERT(r.index == 0, "pcmpestri lowest result index=%u", r.index);
    assert_eflags(r, 1, 0, 0, 1, "lowest result");
    RUN_PCMPESTRI(0x58, a, b, 16, 16, r);
    TEST_ASSERT(r.index == 15, "pcmpestri highest result index=%u", r.index);
    assert_eflags(r, 1, 0, 0, 1, "highest result");

    RUN_PCMPESTRM(0x18, a, b, 16, 16, mr);
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x8001u, "pcmpestrm bit-mask output=0x%04x", mr.mask.u32[0] & 0xffffu);
    RUN_PCMPESTRM(0x58, a, b, 16, 16, mr);
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(mr.mask.u8[i] == ((i == 0 || i == 15) ? 0xff : 0), "pcmpestrm unit-mask lane %d", i);
    TEST_ASSERT(mr.cf == 1 && mr.zf == 0 && mr.sf == 0 && mr.of == 1,
        "pcmpestrm flags CF=%u ZF=%u SF=%u OF=%u", mr.cf, mr.zf, mr.sf, mr.of);
}

int main(void) {
    TEST_START("PCMPESTRI/PCMPESTRM instructions (SSE4.2)");
    test_pcmpestri_equal_each();
    test_pcmpestri_all_match();
    test_pcmpestri_partial_length();
    test_pcmpestrm_equal_each();
    test_pcmpestri_flags();
    test_pcmpestri_length_boundaries();
    test_pcmpestri_controls();
    TEST_END();
}
