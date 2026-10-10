/*
 * ADC16 deferred-flags regression: a 17-bit sum of 0x10000 must set ZF.
 * An indirect jump separates ADC from flag capture so translators must carry
 * the flags across a block boundary instead of only computing them locally.
 *
 * Box64 RV64 currently emits d_adc16, not d_adc16b. These architectural tests
 * also apply to a translator using the truncated-result d_adc16b path, but
 * cannot select that internal path from x86 code.
 */
#include "../common.h"

enum adc_form {
    REG_11, REG_13, MEM_SOURCE, MEM_DEST, LOCK_MEM_DEST,
    IMM8_ZERO, IMM8_ONE, IMM8_MINUS_ONE, IMM16_AX, IMM16_GROUP,
    FORM_COUNT
};

static const char *const form_names[] = {
    "reg/opcode11", "reg/opcode13", "memory-source", "memory-dest",
    "lock-memory-dest", "imm8/0", "imm8/1", "imm8/-1",
    "imm16/AX", "imm16/group81"
};

static uint64_t expected_flags(uint16_t a, uint16_t b, unsigned carry) {
    uint32_t sum = (uint32_t)a + b + carry;
    uint16_t result = (uint16_t)sum;
    unsigned ones = 0;
    for (unsigned bit = 0; bit < 8; ++bit)
        ones += (result >> bit) & 1;

    uint64_t flags = 0;
    if (sum > UINT16_MAX) flags |= CF_FLAG;
    if (!(ones & 1)) flags |= PF_FLAG;
    if ((a & 15u) + (b & 15u) + carry > 15) flags |= AF_FLAG;
    if (!result) flags |= ZF_FLAG;
    if (result & 0x8000) flags |= SF_FLAG;
    if ((~(a ^ b) & (a ^ result)) & 0x8000) flags |= OF_FLAG;
    return flags;
}

static void check_case(enum adc_form form, uint16_t a, uint16_t b,
                       unsigned carry) {
    const uint64_t upper = UINT64_C(0x123456789abc0000);
    uint64_t result = upper | a;
    uint64_t flags;
    struct {
        uint64_t dest;
        uint16_t source;
    } mem = { result, b };

    /* All inputs are consumed before either output can overwrite them.
     * MOV/LEA/JMP preserve ADC flags; flag capture stays in this asm block.
     */
#define RUN_ADC(instruction, read_result) \
    __asm__ volatile ( \
        "leaq 1f(%%rip), %%r11\n\t" \
        "btl $0, %%ecx\n\t" \
        instruction "\n\t" \
        "jmp *%%r11\n\t" \
        "1: pushfq\n\t" \
        "popq %%r10\n\t" \
        read_result \
        "movq %%r10, %1" \
        : "+a"(result), "=r"(flags) \
        : "d"((uint64_t)b), "c"((uint64_t)carry), "D"(&mem) \
        : "r10", "r11", "cc", "memory" \
    )

    switch (form) {
        case REG_11:
            RUN_ADC(".byte 0x66, 0x11, 0xd0", "");
            break;
        case REG_13:
            RUN_ADC(".byte 0x66, 0x13, 0xc2", "");
            break;
        case MEM_SOURCE:
            RUN_ADC("adcw 8(%%rdi), %%ax", "");
            break;
        case MEM_DEST:
            RUN_ADC("adcw %%dx, (%%rdi)", "movq (%%rdi), %%rax\n\t");
            break;
        case LOCK_MEM_DEST:
            RUN_ADC("lock adcw %%dx, (%%rdi)", "movq (%%rdi), %%rax\n\t");
            break;
        case IMM8_ZERO:
            RUN_ADC(".byte 0x66, 0x83, 0xd0, 0x00", "");
            break;
        case IMM8_ONE:
            RUN_ADC(".byte 0x66, 0x83, 0xd0, 0x01", "");
            break;
        case IMM8_MINUS_ONE:
            RUN_ADC(".byte 0x66, 0x83, 0xd0, 0xff", "");
            break;
        case IMM16_AX:
            RUN_ADC(".byte 0x66, 0x15, 0x00, 0x80", "");
            break;
        case IMM16_GROUP:
            RUN_ADC(".byte 0x66, 0x81, 0xd0, 0x00, 0x80", "");
            break;
        default:
            abort();
    }
#undef RUN_ADC

    uint16_t expected = (uint16_t)((uint32_t)a + b + carry);
    uint64_t want_flags = expected_flags(a, b, carry);
    TEST_ASSERT((uint16_t)result == expected,
                "%s %04x+%04x+%u: result=%04x expected=%04x",
                form_names[form], a, b, carry, (uint16_t)result, expected);
    TEST_ASSERT((result & ~UINT64_C(0xffff)) == upper,
                "%s %04x+%04x+%u: upper destination bits changed",
                form_names[form], a, b, carry);

    static const uint64_t masks[] = {
        CF_FLAG, PF_FLAG, AF_FLAG, ZF_FLAG, SF_FLAG, OF_FLAG
    };
    static const char *const flag_names[] = { "CF", "PF", "AF", "ZF", "SF", "OF" };
    for (unsigned i = 0; i < sizeof(masks) / sizeof(masks[0]); ++i) {
        TEST_ASSERT((flags & masks[i]) == (want_flags & masks[i]),
                    "%s %04x+%04x+%u: %s=%u expected=%u (flags=0x%llx)",
                    form_names[form], a, b, carry, flag_names[i],
                    !!(flags & masks[i]), !!(want_flags & masks[i]),
                    (unsigned long long)flags);
    }
}

int main(void) {
    TEST_START("ADC16 deferred flags across an indirect jump");
    static const uint16_t values[] = {
        0x0000, 0x0001, 0x000e, 0x000f, 0x0010, 0x007f, 0x0080,
        0x00ff, 0x0100, 0x7ffe, 0x7fff, 0x8000, 0x8001,
        0xff00, 0xfffe, 0xffff
    };
    const unsigned count = sizeof(values) / sizeof(values[0]);
    for (enum adc_form form = REG_11; form < FORM_COUNT; ++form) {
        for (unsigned carry = 0; carry <= 1; ++carry) {
            for (unsigned i = 0; i < count; ++i) {
                if (form <= LOCK_MEM_DEST) {
                    for (unsigned j = 0; j < count; ++j)
                        check_case(form, values[i], values[j], carry);
                    /* Explicit wrap-to-zero pairs, with and without carry-in. */
                    check_case(form, values[i],
                               (uint16_t)(0u - values[i] - carry), carry);
                } else {
                    uint16_t b = form == IMM8_ZERO ? 0 :
                                 form == IMM8_ONE ? 1 :
                                 form == IMM8_MINUS_ONE ? 0xffff : 0x8000;
                    check_case(form, values[i], b, carry);
                    if (i == 0)
                        check_case(form, (uint16_t)(0u - b - carry), b, carry);
                }
            }
        }
    }
    TEST_END();
}
