/* TARGET: VCVTSI2SS */
#define TEST_NAME "VCVTSI2SS"
#define TEST_INSN "vcvtsi2ss"
#define TEST_BITS 32
#define TEST_FIELD f32
#define TEST_FLOAT_T float
#define TEST_TWO63 0x1p63f
#define TEST_ROUND_INPUT INT64_C(16777217)
#define TEST_ROUND_EXPECTED 0x1p24f
#include "../include/avx512_cvtsi2fp_test.h"
