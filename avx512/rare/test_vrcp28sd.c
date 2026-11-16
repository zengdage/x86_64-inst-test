/* TARGET: VRCP28SD */
#define TEST_NAME "VRCP28SD"
#define TEST_INSN "vrcp28sd"
#define TEST_BITS 64
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0 / (value))
#include "../include/avx512_er_scalar_test.h"
