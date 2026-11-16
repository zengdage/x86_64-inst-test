/* TARGET: VMOVSHDUP */
#define TEST_BITS 32
#define TEST_NAME "VMOVSHDUP"
#define TEST_INSN "vmovshdup"
#define TEST_BASE UINT32_C(0x10000000)
#define TEST_SOURCE_INDEX(i) ((i) | 1)
#include "../include/avx512_dup_move_test.h"
