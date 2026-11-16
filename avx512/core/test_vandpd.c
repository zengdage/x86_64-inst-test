/* TARGET: VANDPD */
#define TEST_BITS 64
#define TEST_NAME "VANDPD"
#define TEST_INSN "vandpd"
#define TEST_A_PATTERN UINT64_C(0xf0f0f0f00f0f0f0f)
#define TEST_B_PATTERN UINT64_C(0xff00ff0055aa55aa)
#define TEST_EXPECT(a, b) ((a) & (b))
#include "../include/avx512_fp_logic_test.h"
