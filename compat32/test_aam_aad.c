/* AAM/AAD require 32-bit mode. Check AX, preserved EAX[31:16], and SF/ZF/PF.
 * CF/OF/AF are undefined and deliberately excluded from the oracle.
 */
#include "../common.h"

#if !defined(__i386__)
#error "Build this test with -m32"
#endif

static void check_result(const char *instruction, unsigned base, uint32_t input,
                         uint32_t result, uint32_t flags, uint32_t expected) {
    unsigned al = expected & 0xff;
    unsigned parity = 0;
    for (unsigned bits = al; bits; bits >>= 1)
        parity ^= bits & 1;
    uint32_t expected_flags = (al & 0x80 ? SF_FLAG : 0) |
                              (al == 0 ? ZF_FLAG : 0) |
                              (parity == 0 ? PF_FLAG : 0);
    TEST_ASSERT(result == expected,
                "%s base %u input 0x%08x: expected EAX 0x%08x, got 0x%08x",
                instruction, base, input, expected, result);
    TEST_ASSERT((flags & (SF_FLAG | ZF_FLAG | PF_FLAG)) == expected_flags,
                "%s base %u input 0x%08x: expected SF/ZF/PF 0x%x, got 0x%x",
                instruction, base, input, expected_flags,
                (unsigned)(flags & (SF_FLAG | ZF_FLAG | PF_FLAG)));
}

#define DEFINE_AAM(BASE) \
static void test_aam_##BASE(void) { \
    for (unsigned al = 0; al < 256; ++al) { \
        uint32_t upper = (al & 1) ? UINT32_C(0xabcd0000) : UINT32_C(0x12340000); \
        uint32_t input = upper | 0xff00 | al; \
        uint32_t result, flags; \
        __asm__ volatile ( \
            "aam $" #BASE "\n\t" \
            "pushfl\n\t" \
            "popl %1" \
            : "=a"(result), "=&r"(flags) : "0"(input) : "cc" \
        ); \
        uint32_t expected = upper | ((al / BASE) << 8) | (al % BASE); \
        check_result("aam", BASE, input, result, flags, expected); \
    } \
}

#define DEFINE_AAD(BASE) \
static void test_aad_##BASE(void) { \
    static const unsigned ah_values[] = {0, 1, 9, 0x7f, 0x80, 0xff}; \
    for (unsigned i = 0; i < sizeof(ah_values) / sizeof(ah_values[0]); ++i) { \
        for (unsigned al = 0; al < 256; ++al) { \
            uint32_t upper = (al & 1) ? UINT32_C(0xcafe0000) : UINT32_C(0x56780000); \
            uint32_t input = upper | (ah_values[i] << 8) | al; \
            uint32_t result, flags; \
            __asm__ volatile ( \
                "aad $" #BASE "\n\t" \
                "pushfl\n\t" \
                "popl %1" \
                : "=a"(result), "=&r"(flags) : "0"(input) : "cc" \
            ); \
            uint32_t expected = upper | ((ah_values[i] * BASE + al) & 0xff); \
            check_result("aad", BASE, input, result, flags, expected); \
        } \
    } \
}

DEFINE_AAM(1)
DEFINE_AAM(10)
DEFINE_AAM(16)
DEFINE_AAM(255)
DEFINE_AAD(0)
DEFINE_AAD(1)
DEFINE_AAD(10)
DEFINE_AAD(16)
DEFINE_AAD(255)

int main(void) {
    TEST_START("AAM/AAD (32-bit)");
    test_aam_1();
    test_aam_10();
    test_aam_16();
    test_aam_255();
    test_aad_0();
    test_aad_1();
    test_aad_10();
    test_aad_16();
    test_aad_255();
    TEST_END();
}
