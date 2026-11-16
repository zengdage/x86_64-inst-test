/* TARGET: VRSQRTPS */
#define TEST_NAME "VRSQRTPS"
#define TEST_INSN "vrsqrtps"
#define TEST_BITS 32
#define TEST_SCALAR 0
#define TEST_UNARY UNARY_RSQRT
#include "avx_fp_unary_test.h"
