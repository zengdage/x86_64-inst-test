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

int main(void) {
    TEST_START("PCMPESTRI/PCMPESTRM instructions (SSE4.2)");
    test_pcmpestri_equal_each();
    test_pcmpestri_all_match();
    test_pcmpestri_partial_length();
    test_pcmpestrm_equal_each();
    test_pcmpestri_flags();
    TEST_END();
}
