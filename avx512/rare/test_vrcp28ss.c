/* TARGET: VRCP28SS */
#define TEST_NAME "VRCP28SS"
#define TEST_INSN "vrcp28ss"
#define TEST_BITS 32
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0f / (value))
#include "../include/avx512_er_scalar_test.h"
