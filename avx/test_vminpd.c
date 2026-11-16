/* TARGET: VMINPD */
#define TEST_NAME "VMINPD"
#define TEST_INSN "vminpd"
#define TEST_BITS 64
#define TEST_SCALAR 0
#define TEST_OP FP_OP_MIN
#include "avx_fp_binary_test.h"
