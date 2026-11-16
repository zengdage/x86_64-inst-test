/* TARGET: VMOVSLDUP */
#define TEST_BITS 32
#define TEST_NAME "VMOVSLDUP"
#define TEST_INSN "vmovsldup"
#define TEST_BASE UINT32_C(0x20000000)
#define TEST_SOURCE_INDEX(i) ((i) & ~1)
#include "../include/avx512_dup_move_test.h"
