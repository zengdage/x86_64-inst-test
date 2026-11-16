/* TARGET: VFNMADD132SD */
#define TEST_NAME "VFNMADD132SD"
#define TEST_INSN "vfnmadd132sd"
#define TEST_BITS 64
#define TEST_SCALAR 1
#define TEST_FORM 132
#define TEST_KIND FMA_KIND_NMADD
#include "avx_fma_test.h"
