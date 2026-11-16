/* TARGET: VFNMADD132PS */
#define TEST_NAME "VFNMADD132PS"
#define TEST_INSN "vfnmadd132ps"
#define TEST_BITS 32
#define TEST_SCALAR 0
#define TEST_FORM 132
#define TEST_KIND FMA_KIND_NMADD
#include "avx_fma_test.h"
