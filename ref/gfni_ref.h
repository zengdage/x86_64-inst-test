#ifndef X86_64_INST_TEST_GFNI_REF_H
#define X86_64_INST_TEST_GFNI_REF_H

#include <stdint.h>

static inline uint8_t gfni_mul_reference(uint8_t a, uint8_t b) {
    uint8_t result = 0;
    for (int bit = 0; bit < 8; bit++) {
        if (b & 1U) result ^= a;
        a = (uint8_t)((a << 1) ^ ((a & 0x80U) ? 0x1bU : 0U));
        b >>= 1;
    }
    return result;
}

static inline uint8_t gfni_inverse_reference(uint8_t value) {
    uint8_t result = 1;
    uint8_t base = value;
    unsigned exponent = 254;
    if (value == 0) return 0;
    while (exponent != 0) {
        if (exponent & 1U) result = gfni_mul_reference(result, base);
        base = gfni_mul_reference(base, base);
        exponent >>= 1;
    }
    return result;
}

#endif
