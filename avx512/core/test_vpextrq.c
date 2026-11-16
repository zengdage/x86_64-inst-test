/* TARGET: VPEXTRQ */
#define TEST_NAME "VPEXTRQ"
#define TEST_INSN "vpextrq"
#define TEST_FIELD u64
#define TEST_VALUE_T uint64_t
#define TEST_REG_T uint64_t
#define TEST_LANES 2
#define TEST_LAST_IMM 1
#define TEST_LAST_TEXT "1"
#define TEST_BASE UINT64_C(0x8000000000000000)
#define CHECK_FEATURE() CHECK_AVX512DQ()
#include "../include/avx512_pextr_test.h"
