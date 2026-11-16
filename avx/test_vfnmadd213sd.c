/* TARGET: VFNMADD213SD */
#define TEST_NAME "VFNMADD213SD"
#define TEST_INSN "vfnmadd213sd"
#define TEST_BITS 64
#define TEST_SCALAR 1
#define TEST_FORM 213
#define TEST_KIND FMA_KIND_NMADD
#include "avx_fma_test.h"
