/* TARGET: VRSQRT28SS */
#define TEST_NAME "VRSQRT28SS"
#define TEST_INSN "vrsqrt28ss"
#define TEST_BITS 32
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0f / sqrtf(value))
#include "../include/avx512_er_scalar_test.h"
