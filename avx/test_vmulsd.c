/* TARGET: VMULSD */
#define TEST_NAME "VMULSD"
#define TEST_INSN "vmulsd"
#define TEST_BITS 64
#define TEST_SCALAR 1
#define TEST_OP FP_OP_MUL
#include "avx_fp_binary_test.h"
