/*
 * test_pcmpistri.c - Test PCMPISTRI/PCMPISTRM instructions (SSE4.2)
 *
 * PCMPISTRI: Packed compare implicit-length strings, return index in ECX.
 * PCMPISTRM: Packed compare implicit-length strings, return mask in XMM0.
 * String length is implicit (determined by null terminator in the data).
 * imm8 controls: [1:0]=format, [3:2]=aggregation, [5:4]=polarity, [6]=output select
 *
 * Compile: gcc -o test_pcmpistri simd/test_pcmpistri.c -O0 -msse4.2
 * Note: Do not use static linking.
 */
#include "../common.h"

typedef struct {
    uint32_t index;
    uint8_t cf, zf, sf, of;
} pcmp_i_result;

typedef struct {
    xmm_t mask;
    uint8_t cf, zf, sf, of;
} pcmp_m_result;

#define RUN_PCMPISTRI(imm, lhs, rhs, result) do { \
    __asm__ volatile ( \
        "movdqa %5, %%xmm1\n\t" \
        "movdqa %6, %%xmm2\n\t" \
        "pcmpistri $" #imm ", %%xmm2, %%xmm1\n\t" \
        "movl %%ecx, %0\n\t" \
        "setc %1\n\t" \
        "setz %2\n\t" \
        "sets %3\n\t" \
        "seto %4" \
        : "=&r"((result).index), "=qm"((result).cf), "=qm"((result).zf), \
          "=qm"((result).sf), "=qm"((result).of) \
        : "m"(lhs), "m"(rhs) : "xmm1", "xmm2", "ecx", "cc"); \
} while (0)

#define RUN_PCMPISTRM(imm, lhs, rhs, result) do { \
    __asm__ volatile ( \
        "movdqa %5, %%xmm1\n\t" \
        "movdqa %6, %%xmm2\n\t" \
        "pcmpistrm $" #imm ", %%xmm2, %%xmm1\n\t" \
        "movdqa %%xmm0, %0\n\t" \
        "setc %1\n\t" \
        "setz %2\n\t" \
        "sets %3\n\t" \
        "seto %4" \
        : "=m"((result).mask), "=qm"((result).cf), "=qm"((result).zf), \
          "=qm"((result).sf), "=qm"((result).of) \
        : "m"(lhs), "m"(rhs) : "xmm0", "xmm1", "xmm2", "cc"); \
} while (0)

static void assert_flags(pcmp_i_result r, int cf, int zf, int sf, int of, const char *name) {
    TEST_ASSERT(r.cf == cf, "%s CF: expected %d, got %u", name, cf, r.cf);
    TEST_ASSERT(r.zf == zf, "%s ZF: expected %d, got %u", name, zf, r.zf);
    TEST_ASSERT(r.sf == sf, "%s SF: expected %d, got %u", name, sf, r.sf);
    TEST_ASSERT(r.of == of, "%s OF: expected %d, got %u", name, of, r.of);
}

static void test_pcmpistri_equal_each(void) {
    /* Equal each: compare corresponding bytes */
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "Hello World!", 12);
    memcpy(b.u8, "Hello World!", 12);

    uint32_t idx;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "pcmpistri $0x18, %%xmm2, %%xmm1\n\t"  /* equal each, negative polarity */
        "movl %%ecx, %0"
        : "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "ecx", "cc"
    );
    /* All match => negative polarity makes all 0, index = 16 (no bit set) */
    TEST_ASSERT(idx == 16, "pcmpistri equal each all match: idx expected 16, got %u", idx);
}

static void test_pcmpistri_find_mismatch(void) {
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "ABCDEFGH", 8);
    memcpy(b.u8, "ABCXEFGH", 8);

    uint32_t idx;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "pcmpistri $0x18, %%xmm2, %%xmm1\n\t"  /* equal each, negative polarity */
        "movl %%ecx, %0"
        : "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "ecx", "cc"
    );
    /* Mismatch at index 3 */
    TEST_ASSERT(idx == 3, "pcmpistri find mismatch: expected 3, got %u", idx);
}

static void test_pcmpistri_ranges(void) {
    /* Ranges mode: check if chars in b are in ranges defined by a */
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    /* Range: 'a'-'z' */
    a.u8[0] = 'a'; a.u8[1] = 'z';
    memcpy(b.u8, "helloWORLD", 10);

    uint32_t idx;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "pcmpistri $0x04, %%xmm2, %%xmm1\n\t"  /* ranges, positive polarity */
        "movl %%ecx, %0"
        : "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "ecx", "cc"
    );
    /* First lowercase char is at index 0 */
    TEST_ASSERT(idx == 0, "pcmpistri ranges lowercase first: expected 0, got %u", idx);
}

static void test_pcmpistrm_equal_each(void) {
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "ABCD", 4);
    memcpy(b.u8, "AXCX", 4);

    xmm_t result;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "pcmpistrm $0x08, %%xmm2, %%xmm1\n\t"  /* equal each, positive polarity, bit mask */
        "movdqa %%xmm0, %0"
        : "=m"(result) : "m"(a), "m"(b) : "xmm0", "xmm1", "xmm2", "cc"
    );
    /* Matches at positions 0 and 2 => mask = 0x05 */
    TEST_ASSERT((result.u32[0] & 0xF) == 0x05,
        "pcmpistrm equal each: expected mask 0x05, got 0x%x", result.u32[0] & 0xF);
}

static void test_pcmpistri_equal_any(void) {
    /* Equal any: check if any char in b matches any char in a */
    xmm_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "aeiou", 5);  /* vowels */
    memcpy(b.u8, "hxllx wxrld", 11);

    uint32_t idx;
    __asm__ volatile (
        "movdqa %1, %%xmm1\n\t"
        "movdqa %2, %%xmm2\n\t"
        "pcmpistri $0x00, %%xmm2, %%xmm1\n\t"  /* equal any, positive polarity */
        "movl %%ecx, %0"
        : "=r"(idx) : "m"(a), "m"(b) : "xmm1", "xmm2", "ecx", "cc"
    );
    /* No vowels in "hxllx wxrld", so no match. idx should be 16 */
    TEST_ASSERT(idx == 16, "pcmpistri equal any no match: expected 16, got %u", idx);
}

static void test_pcmpistri_length_and_flag_boundaries(void) {
    xmm_t a, b;
    pcmp_i_result r;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    RUN_PCMPISTRI(0x08, a, b, r); /* equal-each, positive */
    TEST_ASSERT(r.index == 0, "both empty positive equal-each index: %u", r.index);
    assert_flags(r, 1, 1, 1, 1, "both empty");

    memset(&a, 0, sizeof(a));
    memset(&b, 'B', sizeof(b));
    RUN_PCMPISTRI(0x08, a, b, r);
    TEST_ASSERT(r.index == 16, "NUL at lhs lane 0 index: %u", r.index);
    assert_flags(r, 0, 0, 1, 0, "lhs NUL lane 0");

    memset(&a, 'A', sizeof(a));
    memset(&b, 0, sizeof(b));
    RUN_PCMPISTRI(0x08, a, b, r);
    TEST_ASSERT(r.index == 16, "NUL at rhs lane 0 index: %u", r.index);
    assert_flags(r, 0, 1, 0, 0, "rhs NUL lane 0");

    memset(&a, 'Q', sizeof(a));
    memset(&b, 'Q', sizeof(b));
    a.u8[15] = 0;
    b.u8[15] = 0;
    RUN_PCMPISTRI(0x18, a, b, r); /* equal-each, negative */
    TEST_ASSERT(r.index == 16, "NUL at lane 15 equal strings index: %u", r.index);
    assert_flags(r, 0, 1, 1, 0, "both NUL lane 15");

    memset(&a, 'Q', sizeof(a));
    memset(&b, 'Q', sizeof(b));
    b.u8[15] = 'X';
    RUN_PCMPISTRI(0x18, a, b, r);
    TEST_ASSERT(r.index == 15, "full 16-byte mismatch at highest lane: %u", r.index);
    assert_flags(r, 1, 0, 0, 0, "full mismatch lane 15");

    b = a;
    RUN_PCMPISTRI(0x18, a, b, r);
    TEST_ASSERT(r.index == 16, "full 16-byte equal input index: %u", r.index);
    assert_flags(r, 0, 0, 0, 0, "full 16-byte equal");
}

static void test_pcmpistri_polarity_and_output_select(void) {
    xmm_t a, b;
    pcmp_i_result r;
    pcmp_m_result mr;

    /* Full lhs and four-byte rhs distinguish negative from masked-negative. */
    memset(&a, 'X', sizeof(a));
    memset(&b, 0, sizeof(b));
    memcpy(a.u8, "ABCD", 4);
    memcpy(b.u8, "ABCD", 4);
    RUN_PCMPISTRM(0x08, a, b, mr); /* positive */
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x000fu,
        "positive polarity mask: 0x%04x", mr.mask.u32[0] & 0xffffu);
    RUN_PCMPISTRM(0x18, a, b, mr); /* negative */
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0xfff0u,
        "negative polarity mask: 0x%04x", mr.mask.u32[0] & 0xffffu);
    RUN_PCMPISTRM(0x28, a, b, mr); /* masked positive */
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x000fu,
        "masked-positive polarity mask: 0x%04x", mr.mask.u32[0] & 0xffffu);
    RUN_PCMPISTRM(0x38, a, b, mr); /* masked negative */
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x0000u,
        "masked-negative polarity mask: 0x%04x", mr.mask.u32[0] & 0xffffu);

    memset(&a, 'K', sizeof(a));
    b = a;
    b.u8[1] = 'x';
    b.u8[14] = 'y';
    RUN_PCMPISTRI(0x18, a, b, r); /* least significant result bit */
    TEST_ASSERT(r.index == 1, "lowest-index selection: %u", r.index);
    assert_flags(r, 1, 0, 0, 0, "lowest-index selection");
    RUN_PCMPISTRI(0x58, a, b, r); /* negative, most significant result bit */
    TEST_ASSERT(r.index == 14, "highest-index selection: %u", r.index);
    assert_flags(r, 1, 0, 0, 0, "highest-index selection");

    RUN_PCMPISTRM(0x18, a, b, mr); /* bit mask */
    TEST_ASSERT((mr.mask.u32[0] & 0xffffu) == 0x4002u,
        "bit-mask output boundary: 0x%04x", mr.mask.u32[0] & 0xffffu);
    TEST_ASSERT(mr.cf == 1 && mr.zf == 0 && mr.sf == 0 && mr.of == 0,
        "bit-mask complete flags CF=%u ZF=%u SF=%u OF=%u", mr.cf, mr.zf, mr.sf, mr.of);
    RUN_PCMPISTRM(0x58, a, b, mr); /* unit mask */
    for (int i = 0; i < 16; i++) {
        uint8_t expected = (i == 1 || i == 14) ? 0xff : 0x00;
        TEST_ASSERT(mr.mask.u8[i] == expected, "unit-mask lane %d: 0x%02x", i, mr.mask.u8[i]);
    }
    TEST_ASSERT(mr.cf == 1 && mr.zf == 0 && mr.sf == 0 && mr.of == 0,
        "unit-mask complete flags CF=%u ZF=%u SF=%u OF=%u", mr.cf, mr.zf, mr.sf, mr.of);

    /* A mismatch at lane zero exercises OF=IntRes2[0]. */
    b = a;
    b.u8[0] = 'z';
    RUN_PCMPISTRI(0x18, a, b, r);
    TEST_ASSERT(r.index == 0, "lane-zero mismatch index: %u", r.index);
    assert_flags(r, 1, 0, 0, 1, "lane-zero mismatch");
}

int main(void) {
    TEST_START("PCMPISTRI/PCMPISTRM instructions (SSE4.2)");
    test_pcmpistri_equal_each();
    test_pcmpistri_find_mismatch();
    test_pcmpistri_ranges();
    test_pcmpistrm_equal_each();
    test_pcmpistri_equal_any();
    test_pcmpistri_length_and_flag_boundaries();
    test_pcmpistri_polarity_and_output_select();
    TEST_END();
}
