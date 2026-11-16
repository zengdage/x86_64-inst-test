#include "../common.h"

#if !defined(__i386__)
#error "Build this test with -m32"
#endif

static unsigned even_parity(uint8_t value) {
    unsigned parity = 0;
    for (; value; value >>= 1)
        parity ^= value & 1;
    return parity == 0;
}

static uint32_t input_flags(unsigned cf, unsigned af) {
    uint32_t flags;
    __asm__ volatile ("pushfl\n\tpopl %0" : "=r"(flags));
    flags &= ~(CF_FLAG | AF_FLAG);
    return flags | (cf ? CF_FLAG : 0) | (af ? AF_FLAG : 0);
}

static void test_aaa_aas(void) {
    static const uint16_t values[] = {0x0000, 0x0009, 0x000a, 0x00ff, 0x1205, 0x12fa, 0xffff};
    for (unsigned op = 0; op < 2; ++op) {
        for (unsigned af = 0; af < 2; ++af) {
            for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
                uint32_t eax_in = UINT32_C(0xa5a50000) | values[i];
                uint32_t eax_out, flags_out;
                uint32_t flags_in = input_flags(0, af);
                if (op == 0) {
                    __asm__ volatile (
                        "pushl %[flags]\n\tpopfl\n\taaa\n\tpushfl\n\tpopl %[out_flags]"
                        : "=a"(eax_out), [out_flags] "=&r"(flags_out)
                        : "0"(eax_in), [flags] "r"(flags_in) : "cc");
                } else {
                    __asm__ volatile (
                        "pushl %[flags]\n\tpopfl\n\taas\n\tpushfl\n\tpopl %[out_flags]"
                        : "=a"(eax_out), [out_flags] "=&r"(flags_out)
                        : "0"(eax_in), [flags] "r"(flags_in) : "cc");
                }
                uint16_t ax = values[i];
                unsigned adjust = ((ax & 0xf) > 9) || af;
                if (adjust)
                    ax = op == 0 ? (uint16_t)(ax + 0x106) : (uint16_t)(ax - 0x106);
                ax &= UINT16_C(0xff0f);
                uint32_t expected = (eax_in & UINT32_C(0xffff0000)) | ax;
                TEST_ASSERT(eax_out == expected, "%s AX=%#x AF=%u: got EAX=%#x",
                            op == 0 ? "aaa" : "aas", values[i], af, eax_out);
                TEST_ASSERT(!!(flags_out & AF_FLAG) == adjust &&
                            !!(flags_out & CF_FLAG) == adjust,
                            "%s AX=%#x AF/CF result", op == 0 ? "aaa" : "aas", values[i]);
            }
        }
    }
}

static uint8_t daa_reference(uint8_t al, unsigned old_cf, unsigned old_af,
                             unsigned *cf, unsigned *af) {
    uint8_t old_al = al;
    if ((al & 0xf) > 9 || old_af) {
        al = (uint8_t)(al + 6);
        *af = 1;
    } else {
        *af = 0;
    }
    if (old_al > 0x99 || old_cf) {
        al = (uint8_t)(al + 0x60);
        *cf = 1;
    } else {
        *cf = 0;
    }
    return al;
}

static uint8_t das_reference(uint8_t al, unsigned old_cf, unsigned old_af,
                             unsigned *cf, unsigned *af) {
    uint8_t old_al = al;
    unsigned lower_borrow = 0;
    if ((al & 0xf) > 9 || old_af) {
        lower_borrow = al < 6;
        al = (uint8_t)(al - 6);
        *af = 1;
    } else {
        *af = 0;
    }
    if (old_al > 0x99 || old_cf) {
        al = (uint8_t)(al - 0x60);
        *cf = 1;
    } else {
        *cf = lower_borrow;
    }
    return al;
}

static void test_daa_das(void) {
    for (unsigned op = 0; op < 2; ++op) {
        for (unsigned cf = 0; cf < 2; ++cf) {
            for (unsigned af = 0; af < 2; ++af) {
                for (unsigned al = 0; al < 256; ++al) {
                    uint32_t eax_in = UINT32_C(0x5a5a9b00) | al;
                    uint32_t eax_out, flags_out;
                    uint32_t flags_in = input_flags(cf, af);
                    if (op == 0) {
                        __asm__ volatile (
                            "pushl %[flags]\n\tpopfl\n\tdaa\n\tpushfl\n\tpopl %[out_flags]"
                            : "=a"(eax_out), [out_flags] "=&r"(flags_out)
                            : "0"(eax_in), [flags] "r"(flags_in) : "cc");
                    } else {
                        __asm__ volatile (
                            "pushl %[flags]\n\tpopfl\n\tdas\n\tpushfl\n\tpopl %[out_flags]"
                            : "=a"(eax_out), [out_flags] "=&r"(flags_out)
                            : "0"(eax_in), [flags] "r"(flags_in) : "cc");
                    }
                    unsigned expected_cf, expected_af;
                    uint8_t expected_al = op == 0
                        ? daa_reference((uint8_t)al, cf, af, &expected_cf, &expected_af)
                        : das_reference((uint8_t)al, cf, af, &expected_cf, &expected_af);
                    uint32_t expected = (eax_in & UINT32_C(0xffffff00)) | expected_al;
                    uint32_t expected_flags = (expected_cf ? CF_FLAG : 0) |
                                              (expected_af ? AF_FLAG : 0) |
                                              (expected_al & 0x80 ? SF_FLAG : 0) |
                                              (expected_al == 0 ? ZF_FLAG : 0) |
                                              (even_parity(expected_al) ? PF_FLAG : 0);
                    TEST_ASSERT(eax_out == expected, "%s AL=%#x CF=%u AF=%u result",
                                op == 0 ? "daa" : "das", al, cf, af);
                    TEST_ASSERT((flags_out & (CF_FLAG | AF_FLAG | SF_FLAG | ZF_FLAG | PF_FLAG)) ==
                                expected_flags,
                                "%s AL=%#x CF=%u AF=%u flags: got %#x expected %#x",
                                op == 0 ? "daa" : "das", al, cf, af,
                                flags_out, expected_flags);
                }
            }
        }
    }
}

int main(void) {
    TEST_START("AAA/AAS/DAA/DAS (32-bit)");
    test_aaa_aas();
    test_daa_das();
    TEST_END();
}
