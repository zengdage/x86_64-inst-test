/* TARGET: VRCP28PS */
#define TEST_NAME "VRCP28PS"
#define TEST_INSN "vrcp28ps"
#define TEST_BITS 32
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0f / (value))
#include "../include/avx512_er_packed_test.h"
