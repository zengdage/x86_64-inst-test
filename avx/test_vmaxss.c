/* TARGET: VMAXSS */
#define TEST_NAME "VMAXSS"
#define TEST_INSN "vmaxss"
#define TEST_BITS 32
#define TEST_SCALAR 1
#define TEST_OP FP_OP_MAX
#include "avx_fp_binary_test.h"
