/* TARGET: VUCOMISD */
#define TEST_NAME "VUCOMISD"
#define TEST_INSN "vucomisd"
#define TEST_MOVE "vmovsd"
#define TEST_VALUE_T double
#define TEST_BITS_T uint64_t
#define TEST_NAN_BITS UINT64_C(0x7ff0123456789abc)
#include "../include/avx512_comi_test.h"
