#ifndef X86_64_AVX_PERM_TEST_H
#define X86_64_AVX_PERM_TEST_H

#include "../common.h"

#define PERM_IMM_QWORD 1
#define PERM_VARIABLE_DWORD 2
#define PERM_LANE_DWORD 3

int main(void) {
    ymm_t data = { .u32 = {10,11,12,13,14,15,16,17} };
#if TEST_PERM != PERM_IMM_QWORD
    ymm_t index = { .u32 = {7,6,5,4,3,2,9,UINT32_MAX} };
#endif
    ymm_t out;
    TEST_START(TEST_NAME);
#if TEST_PERM == PERM_IMM_QWORD
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" TEST_INSN " $0x1b, %%ymm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(data) : "ymm0", "ymm1");
    for (int i = 0; i < 4; i++)
        TEST_ASSERT(out.u64[i] == data.u64[3 - i], TEST_NAME " reverse qword %d", i);
    __asm__ volatile(TEST_INSN " $0, %1, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(data) : "ymm1");
    for (int i = 0; i < 4; i++) TEST_ASSERT(out.u64[i] == data.u64[0], TEST_NAME " broadcast %d", i);
#elif TEST_PERM == PERM_VARIABLE_DWORD
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     TEST_INSN " %%ymm0, %%ymm1, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(data), "m"(index) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == data.u32[index.u32[i] & 7U],
                    TEST_NAME " index lane %d", i);
#else
    ymm_t initial;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     "vpermilps $0x1b, %%ymm0, %%ymm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(data) : "ymm0", "ymm1");
    const uint32_t reverse[8] = {13,12,11,10,17,16,15,14};
    for (int i = 0; i < 8; i++) TEST_ASSERT(out.u32[i] == reverse[i], TEST_NAME " immediate lane %d", i);
    index.u32[0] = 3; index.u32[1] = 2; index.u32[2] = 5; index.u32[3] = UINT32_MAX;
    index.u32[4] = 0; index.u32[5] = 1; index.u32[6] = 6; index.u32[7] = 7;
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vpermilps %%ymm1, %%ymm0, %%ymm2\n\tvmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(data), "m"(index) : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 8; i++) {
        int base = (i / 4) * 4;
        TEST_ASSERT(out.u32[i] == data.u32[base + (index.u32[i] & 3U)],
                    TEST_NAME " variable lane %d", i);
    }
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm1\n\tmovdqu %2, %%xmm0\n\t"
                     "vpermilps $0x1b, %%xmm0, %%xmm1\n\tvmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(initial), "m"(data) : "xmm0", "ymm1");
    for (int i = 4; i < 8; i++) TEST_ASSERT(out.u32[i] == 0, TEST_NAME " VEX upper lane %d", i);
#endif
    TEST_END();
}

#endif
