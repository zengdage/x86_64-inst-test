/* TARGET: VRCP28PD */
#define TEST_NAME "VRCP28PD"
#define TEST_INSN "vrcp28pd"
#define TEST_BITS 64
#define TEST_TOLERANCE 0x1p-27
#define TEST_REFERENCE(value) (1.0 / (value))
#include "../include/avx512_er_packed_test.h"
