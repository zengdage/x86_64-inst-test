/* TARGET: VFNMADD231SD */
#define TEST_NAME "VFNMADD231SD"
#define TEST_INSN "vfnmadd231sd"
#define TEST_BITS 64
#define TEST_SCALAR 1
#define TEST_FORM 231
#define TEST_KIND FMA_KIND_NMADD
#include "avx_fma_test.h"
