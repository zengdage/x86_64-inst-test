/* TARGET: VUCOMISS */
#define TEST_NAME "VUCOMISS"
#define TEST_INSN "vucomiss"
#define TEST_MOVE "vmovss"
#define TEST_VALUE_T float
#define TEST_BITS_T uint32_t
#define TEST_NAN_BITS UINT32_C(0x7f812345)
#include "../include/avx512_comi_test.h"
