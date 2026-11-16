/* TARGET: VCVTSD2SI */
#include "../include/avx512_test.h"
#include <limits.h>

#define CONVERT(mode, value, result) do { \
    double input = (value); \
    __asm__ volatile("vmovsd %1, %%xmm0\n\t" \
                     "vcvtsd2si %{" mode "%}, %%xmm0, %0" \
                     : "=r"(result) : "m"(input) : "xmm0"); \
} while (0)

int main(void) {
    TEST_START("VCVTSD2SI");
    if (!CHECK_AVX512F()) return 0;
    int64_t result;
    CONVERT("rn-sae", 2.5, result);
    TEST_ASSERT(result == 2, "VCVTSD2SI nearest ties-to-even");
    CONVERT("rd-sae", 2.5, result);
    TEST_ASSERT(result == 2, "VCVTSD2SI round down positive");
    CONVERT("ru-sae", 2.5, result);
    TEST_ASSERT(result == 3, "VCVTSD2SI round up positive");
    CONVERT("rz-sae", 2.5, result);
    TEST_ASSERT(result == 2, "VCVTSD2SI round toward zero positive");
    CONVERT("rd-sae", -2.5, result);
    TEST_ASSERT(result == -3, "VCVTSD2SI round down negative");
    CONVERT("ru-sae", -2.5, result);
    TEST_ASSERT(result == -2, "VCVTSD2SI round up negative");
    double nan_value;
    uint64_t nan_bits = UINT64_C(0x7ff8123456789abc);
    memcpy(&nan_value, &nan_bits, sizeof(nan_value));
    CONVERT("rn-sae", nan_value, result);
    TEST_ASSERT(result == INT64_MIN, "VCVTSD2SI NaN integer indefinite");
    TEST_END();
}

#undef CONVERT
