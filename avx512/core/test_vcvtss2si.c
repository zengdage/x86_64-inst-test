/* TARGET: VCVTSS2SI */
#include "../include/avx512_test.h"
#include <limits.h>

#define CONVERT(mode, value, result) do { \
    float input = (value); \
    __asm__ volatile("vmovss %1, %%xmm0\n\t" \
                     "vcvtss2si %{" mode "%}, %%xmm0, %0" \
                     : "=r"(result) : "m"(input) : "xmm0"); \
} while (0)

int main(void) {
    TEST_START("VCVTSS2SI");
    if (!CHECK_AVX512F()) return 0;
    int64_t result;
    CONVERT("rn-sae", 2.5f, result);
    TEST_ASSERT(result == 2, "VCVTSS2SI nearest ties-to-even");
    CONVERT("rd-sae", 2.5f, result);
    TEST_ASSERT(result == 2, "VCVTSS2SI round down positive");
    CONVERT("ru-sae", 2.5f, result);
    TEST_ASSERT(result == 3, "VCVTSS2SI round up positive");
    CONVERT("rz-sae", 2.5f, result);
    TEST_ASSERT(result == 2, "VCVTSS2SI round toward zero positive");
    CONVERT("rd-sae", -2.5f, result);
    TEST_ASSERT(result == -3, "VCVTSS2SI round down negative");
    CONVERT("ru-sae", -2.5f, result);
    TEST_ASSERT(result == -2, "VCVTSS2SI round up negative");
    float nan_value;
    uint32_t nan_bits = UINT32_C(0x7fc12345);
    memcpy(&nan_value, &nan_bits, sizeof(nan_value));
    CONVERT("rn-sae", nan_value, result);
    TEST_ASSERT(result == INT64_MIN, "VCVTSS2SI NaN integer indefinite");
    TEST_END();
}

#undef CONVERT
