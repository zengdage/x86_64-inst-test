/* TARGET: VPEXTRD */
#define TEST_NAME "VPEXTRD"
#define TEST_INSN "vpextrd"
#define TEST_FIELD u32
#define TEST_VALUE_T uint32_t
#define TEST_REG_T uint32_t
#define TEST_LANES 4
#define TEST_LAST_IMM 3
#define TEST_LAST_TEXT "3"
#define TEST_BASE UINT32_C(0x80000000)
#define CHECK_FEATURE() CHECK_AVX512DQ()
#include "../include/avx512_pextr_test.h"
