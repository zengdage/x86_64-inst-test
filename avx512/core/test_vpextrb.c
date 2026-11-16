/* TARGET: VPEXTRB */
#define TEST_NAME "VPEXTRB"
#define TEST_INSN "vpextrb"
#define TEST_FIELD u8
#define TEST_VALUE_T uint8_t
#define TEST_REG_T uint32_t
#define TEST_LANES 16
#define TEST_LAST_IMM 15
#define TEST_LAST_TEXT "15"
#define TEST_BASE UINT8_C(0x80)
#define CHECK_FEATURE() CHECK_AVX512BW()
#include "../include/avx512_pextr_test.h"
