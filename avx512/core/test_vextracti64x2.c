/* TARGET: VEXTRACTI64X2 */
#define TEST_NAME "VEXTRACTI64X2"
#define TEST_INSN "vextracti64x2"
#define TEST_BITS 64
#define TEST_BASE UINT64_C(0x100000000)
#define TEST_SENTINEL UINT64_C(0xdeadbeefcafebabe)
#define CHECK_FEATURE() CHECK_AVX512DQ()
#include "../include/avx512_extractx2x4_test.h"
