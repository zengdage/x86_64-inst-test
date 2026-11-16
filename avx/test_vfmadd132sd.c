/* TARGET: VFMADD132SD */
#define TEST_NAME "VFMADD132SD"
#define TEST_INSN "vfmadd132sd"
#define TEST_BITS 64
#define TEST_SCALAR 1
#define TEST_FORM 132
#define TEST_KIND FMA_KIND_ADD
#include "avx_fma_test.h"
