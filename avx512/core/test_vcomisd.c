/* TARGET: VCOMISD */
#define TEST_NAME "VCOMISD"
#define TEST_INSN "vcomisd"
#define TEST_MOVE "vmovsd"
#define TEST_VALUE_T double
#define TEST_BITS_T uint64_t
#define TEST_NAN_BITS UINT64_C(0x7ff8123456789abc)
#include "../include/avx512_comi_test.h"
