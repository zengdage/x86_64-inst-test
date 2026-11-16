/* TARGET: VMAXSD */
#define TEST_NAME "VMAXSD"
#define TEST_INSN "vmaxsd"
#define TEST_BITS 64
#define TEST_SCALAR 1
#define TEST_OP FP_OP_MAX
#include "avx_fp_binary_test.h"
