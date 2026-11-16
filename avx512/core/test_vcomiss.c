/* TARGET: VCOMISS */
#define TEST_NAME "VCOMISS"
#define TEST_INSN "vcomiss"
#define TEST_MOVE "vmovss"
#define TEST_VALUE_T float
#define TEST_BITS_T uint32_t
#define TEST_NAN_BITS UINT32_C(0x7fc12345)
#include "../include/avx512_comi_test.h"
