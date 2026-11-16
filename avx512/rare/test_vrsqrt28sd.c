/* TARGET: VRSQRT28SD */
#define TEST_NAME "VRSQRT28SD"
#define TEST_INSN "vrsqrt28sd"
#define TEST_BITS 64
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0 / sqrt(value))
#include "../include/avx512_er_scalar_test.h"
