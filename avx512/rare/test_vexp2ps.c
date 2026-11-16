/* TARGET: VEXP2PS */
#define TEST_NAME "VEXP2PS"
#define TEST_INSN "vexp2ps"
#define TEST_BITS 32
#define TEST_TOLERANCE 0x1p-22
#define TEST_REFERENCE(value) exp2f(value)
#include "../include/avx512_er_packed_test.h"
