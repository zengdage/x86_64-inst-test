#ifndef X86_64_AVX512_PEXTR_TEST_H
#define X86_64_AVX512_PEXTR_TEST_H

#include "avx512_test.h"

typedef struct {
    uint64_t before;
    TEST_VALUE_T value;
    uint64_t after;
} __attribute__((packed)) test_box_t;

#define TEST_STRINGIFY_INNER(VALUE) #VALUE
#define TEST_STRINGIFY(VALUE) TEST_STRINGIFY_INNER(VALUE)

#define EXTRACT_REG(IMM, OUTPUT) do {                                           \
    __asm__ volatile("vmovdqa64 %1, %%xmm16\n\t"                              \
                     TEST_INSN " $" TEST_STRINGIFY(IMM) ", %%xmm16, %0"       \
                     : "=r"(OUTPUT) : "m"(src) : "xmm16");                   \
} while (0)

int main(void) {
    xmm_t src;
    TEST_REG_T first, last, wrapped;
    test_box_t box = {
        UINT64_C(0x1122334455667788), 0,
        UINT64_C(0x99aabbccddeeff00)
    };
    for (int i = 0; i < TEST_LANES; i++)
        src.TEST_FIELD[i] = (TEST_VALUE_T)(TEST_BASE + (TEST_VALUE_T)i);

    TEST_START(TEST_NAME);
    if (!CHECK_FEATURE()) return 0;
    EXTRACT_REG(0, first);
    EXTRACT_REG(TEST_LAST_IMM, last);
    EXTRACT_REG(255, wrapped);
    TEST_ASSERT((TEST_VALUE_T)first == src.TEST_FIELD[0],
                TEST_NAME " first element register result");
    TEST_ASSERT((TEST_VALUE_T)last == src.TEST_FIELD[TEST_LANES - 1],
                TEST_NAME " last element register result");
    TEST_ASSERT((TEST_VALUE_T)wrapped == src.TEST_FIELD[TEST_LANES - 1],
                TEST_NAME " immediate high bits ignored");

    __asm__ volatile("vmovdqa64 %1, %%xmm16\n\t"
                     TEST_INSN " $" TEST_LAST_TEXT ", %%xmm16, %0"
                     : "=m"(box.value) : "m"(src) : "xmm16", "memory");
    TEST_ASSERT(box.value == src.TEST_FIELD[TEST_LANES - 1],
                TEST_NAME " memory result");
    TEST_ASSERT(box.before == UINT64_C(0x1122334455667788) &&
                box.after == UINT64_C(0x99aabbccddeeff00),
                TEST_NAME " memory footprint");
    TEST_END();
}

#undef EXTRACT_REG
#undef TEST_STRINGIFY
#undef TEST_STRINGIFY_INNER
#endif
