/* TARGET: VRSQRT28PD */
#define TEST_NAME "VRSQRT28PD"
#define TEST_INSN "vrsqrt28pd"
#define TEST_BITS 64
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0 / sqrt(value))
#include "../include/avx512_er_packed_test.h"
