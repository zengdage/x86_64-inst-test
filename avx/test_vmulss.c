/* TARGET: VMULSS */
#define TEST_NAME "VMULSS"
#define TEST_INSN "vmulss"
#define TEST_BITS 32
#define TEST_SCALAR 1
#define TEST_OP FP_OP_MUL
#include "avx_fp_binary_test.h"
