/* TARGET: VPEXTRW */
#define TEST_NAME "VPEXTRW"
#define TEST_INSN "vpextrw"
#define TEST_FIELD u16
#define TEST_VALUE_T uint16_t
#define TEST_REG_T uint32_t
#define TEST_LANES 8
#define TEST_LAST_IMM 7
#define TEST_LAST_TEXT "7"
#define TEST_BASE UINT16_C(0x8000)
#define CHECK_FEATURE() CHECK_AVX512BW()
#include "../include/avx512_pextr_test.h"
