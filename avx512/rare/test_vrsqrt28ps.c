/* TARGET: VRSQRT28PS */
#define TEST_NAME "VRSQRT28PS"
#define TEST_INSN "vrsqrt28ps"
#define TEST_BITS 32
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0f / sqrtf(value))
#include "../include/avx512_er_packed_test.h"
