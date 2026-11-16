/* TARGET: VCVTSI2SD */
#define TEST_NAME "VCVTSI2SD"
#define TEST_INSN "vcvtsi2sd"
#define TEST_BITS 64
#define TEST_FIELD f64
#define TEST_FLOAT_T double
#define TEST_TWO63 0x1p63
#define TEST_ROUND_INPUT INT64_C(9007199254740993)
#define TEST_ROUND_EXPECTED 0x1p53
#include "../include/avx512_cvtsi2fp_test.h"
