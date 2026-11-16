/* TARGET: VFMADD213SD */
#define TEST_NAME "VFMADD213SD"
#define TEST_INSN "vfmadd213sd"
#define TEST_BITS 64
#define TEST_SCALAR 1
#define TEST_FORM 213
#define TEST_KIND FMA_KIND_ADD
#include "avx_fma_test.h"
