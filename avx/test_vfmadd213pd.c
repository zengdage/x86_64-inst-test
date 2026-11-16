/* TARGET: VFMADD213PD */
#define TEST_NAME "VFMADD213PD"
#define TEST_INSN "vfmadd213pd"
#define TEST_BITS 64
#define TEST_SCALAR 0
#define TEST_FORM 213
#define TEST_KIND FMA_KIND_ADD
#include "avx_fma_test.h"
