/* TARGET: VCVTSD2USI */
#include "../include/avx512_test.h"

#define CONVERT(mode, value, result) do { \
    double input = (value); \
    __asm__ volatile("vmovsd %1, %%xmm0\n\t" \
                     "vcvtsd2usi %{" mode "%}, %%xmm0, %0" \
                     : "=r"(result) : "m"(input) : "xmm0"); \
} while (0)

int main(void) {
    TEST_START("VCVTSD2USI");
    if (!CHECK_AVX512F()) return 0;
    uint64_t result;
    CONVERT("rn-sae", 2.5, result);
    TEST_ASSERT(result == 2, "VCVTSD2USI nearest ties-to-even");
    CONVERT("rd-sae", 2.5, result);
    TEST_ASSERT(result == 2, "VCVTSD2USI round down");
    CONVERT("ru-sae", 2.5, result);
    TEST_ASSERT(result == 3, "VCVTSD2USI round up");
    CONVERT("rz-sae", 2.5, result);
    TEST_ASSERT(result == 2, "VCVTSD2USI round toward zero");
    CONVERT("rn-sae", 4294967296.0, result);
    TEST_ASSERT(result == UINT64_C(4294967296), "VCVTSD2USI exceeds uint32");
    double nan_value;
    uint64_t nan_bits = UINT64_C(0x7ff8123456789abc);
    memcpy(&nan_value, &nan_bits, sizeof(nan_value));
    CONVERT("rn-sae", nan_value, result);
    TEST_ASSERT(result == UINT64_MAX, "VCVTSD2USI NaN integer indefinite");
    TEST_END();
}

#undef CONVERT
