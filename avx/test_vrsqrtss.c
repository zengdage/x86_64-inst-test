/* TARGET: VRSQRTSS */
#define TEST_NAME "VRSQRTSS"
#define TEST_INSN "vrsqrtss"
#define TEST_BITS 32
#define TEST_SCALAR 1
#define TEST_UNARY UNARY_RSQRT
#include "avx_fp_unary_test.h"
