/* TARGET: VEXTRACTPS */
#include "../include/avx512_test.h"

#define EXTRACT(IMM, RESULT) do { \
    __asm__ volatile("vmovdqa32 %1, %%xmm16\n\t" \
                     "vextractps $" #IMM ", %%xmm16, %0" \
                     : "=r"(RESULT) : "m"(src) : "xmm16"); \
} while (0)

int main(void) {
    TEST_START("VEXTRACTPS EVEX");
    if (!CHECK_AVX512F()) return 0;
    const xmm_t src = {.u32 = {
        UINT32_C(0xdeadbeef), UINT32_C(0x12345678),
        UINT32_C(0xcafebabe), UINT32_C(0xfeedface)
    }};
    uint32_t result;
    EXTRACT(0, result);
    TEST_ASSERT(result == src.u32[0], "VEXTRACTPS lane 0");
    EXTRACT(1, result);
    TEST_ASSERT(result == src.u32[1], "VEXTRACTPS lane 1");
    EXTRACT(2, result);
    TEST_ASSERT(result == src.u32[2], "VEXTRACTPS lane 2");
    EXTRACT(3, result);
    TEST_ASSERT(result == src.u32[3], "VEXTRACTPS lane 3");

    uint64_t wide;
    __asm__ volatile("movq $-1, %%rax\n\tvmovdqa32 %1, %%xmm16\n\t"
                     "vextractps $3, %%xmm16, %%eax\n\tmovq %%rax, %0"
                     : "=r"(wide) : "m"(src) : "rax", "xmm16");
    TEST_ASSERT(wide == src.u32[3], "VEXTRACTPS zero-extends GPR result");

    uint32_t memory_result = 0;
    __asm__ volatile("vmovdqa32 %1, %%xmm16\n\t"
                     "vextractps $2, %%xmm16, %0"
                     : "=m"(memory_result) : "m"(src) : "xmm16");
    TEST_ASSERT(memory_result == src.u32[2], "VEXTRACTPS memory result");
    TEST_END();
}

#undef EXTRACT
