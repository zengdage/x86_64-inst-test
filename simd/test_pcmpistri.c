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

int main(void) {
    TEST_START("PCMPISTRI/PCMPISTRM instructions (SSE4.2)");
    test_pcmpistri_equal_each();
    test_pcmpistri_find_mismatch();
    test_pcmpistri_ranges();
    test_pcmpistrm_equal_each();
    test_pcmpistri_equal_any();
    TEST_END();
}
