/* TARGET: VFMADD231PS */
#define TEST_NAME "VFMADD231PS"
#define TEST_INSN "vfmadd231ps"
#define TEST_BITS 32
#define TEST_SCALAR 0
#define TEST_FORM 231
#define TEST_KIND FMA_KIND_ADD
#include "avx_fma_test.h"
