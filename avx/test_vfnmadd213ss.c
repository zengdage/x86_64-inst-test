/* TARGET: VFNMADD213SS */
#define TEST_NAME "VFNMADD213SS"
#define TEST_INSN "vfnmadd213ss"
#define TEST_BITS 32
#define TEST_SCALAR 1
#define TEST_FORM 213
#define TEST_KIND FMA_KIND_NMADD
#include "avx_fma_test.h"
