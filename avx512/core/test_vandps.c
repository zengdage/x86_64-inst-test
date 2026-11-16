/* TARGET: VANDPS */
#define TEST_BITS 32
#define TEST_NAME "VANDPS"
#define TEST_INSN "vandps"
#define TEST_A_PATTERN UINT32_C(0xf0f00f0f)
#define TEST_B_PATTERN UINT32_C(0xff0055aa)
#define TEST_EXPECT(a, b) ((a) & (b))
#include "../include/avx512_fp_logic_test.h"
