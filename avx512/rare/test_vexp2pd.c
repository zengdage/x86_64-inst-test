/* TARGET: VEXP2PD */
#define TEST_NAME "VEXP2PD"
#define TEST_INSN "vexp2pd"
#define TEST_BITS 64
#define TEST_TOLERANCE 0x1p-22
#define TEST_REFERENCE(value) exp2(value)
#include "../include/avx512_er_packed_test.h"
