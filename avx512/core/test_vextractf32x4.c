/* TARGET: VEXTRACTF32X4 */
#define TEST_NAME "VEXTRACTF32X4"
#define TEST_INSN "vextractf32x4"
#define TEST_BITS 32
#define TEST_BASE UINT32_C(0x7fc00000)
#define TEST_SENTINEL UINT32_C(0xdeadbeef)
#define CHECK_FEATURE() CHECK_AVX512F()
#include "../include/avx512_extractx2x4_test.h"
