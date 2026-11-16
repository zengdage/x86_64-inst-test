/* TARGET: VFNMADD231PD */
#define TEST_NAME "VFNMADD231PD"
#define TEST_INSN "vfnmadd231pd"
#define TEST_BITS 64
#define TEST_SCALAR 0
#define TEST_FORM 231
#define TEST_KIND FMA_KIND_NMADD
#include "avx_fma_test.h"
