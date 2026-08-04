#ifndef X86_64_INST_TEST_CLMUL_REF_H
#define X86_64_INST_TEST_CLMUL_REF_H

#include <stdint.h>

static inline void clmul64_reference(uint64_t a, uint64_t b,
                                     uint64_t *low, uint64_t *high) {
    *low = 0;
    *high = 0;
    for (int bit = 0; bit < 64; bit++) {
        if ((b >> bit) & 1U) {
            *low ^= a << bit;
            if (bit != 0) *high ^= a >> (64 - bit);
        }
    }
}

#endif
