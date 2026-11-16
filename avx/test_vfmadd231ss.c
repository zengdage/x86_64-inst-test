/* TARGET: VFMADD231SS */
#define TEST_NAME "VFMADD231SS"
#define TEST_INSN "vfmadd231ss"
#define TEST_BITS 32
#define TEST_SCALAR 1
#define TEST_FORM 231
#define TEST_KIND FMA_KIND_ADD
#include "avx_fma_test.h"
