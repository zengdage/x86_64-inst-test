/* TARGET: VMOVDDUP */
#define TEST_BITS 64
#define TEST_NAME "VMOVDDUP"
#define TEST_INSN "vmovddup"
#define TEST_BASE UINT64_C(0x1000000000000000)
#define TEST_SOURCE_INDEX(i) ((i) & ~1)
#include "../include/avx512_dup_move_test.h"
