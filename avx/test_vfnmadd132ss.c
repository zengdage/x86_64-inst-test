/* TARGET: VFNMADD132SS */
#define TEST_NAME "VFNMADD132SS"
#define TEST_INSN "vfnmadd132ss"
#define TEST_BITS 32
#define TEST_SCALAR 1
#define TEST_FORM 132
#define TEST_KIND FMA_KIND_NMADD
#include "avx_fma_test.h"
