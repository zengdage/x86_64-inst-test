/* TARGET: VINSERTPS */
#include "../include/avx512_test.h"

#define INSERT(IMM, RESULT) do { \
    __asm__ volatile("vmovdqa32 %1, %%xmm16\n\tvmovdqa32 %2, %%xmm17\n\t" \
                     "vinsertps $" #IMM ", %%xmm17, %%xmm16, %%xmm18\n\t" \
                     "vmovdqa32 %%xmm18, %0" \
                     : "=m"(RESULT) : "m"(base), "m"(source) \
                     : "xmm16", "xmm17", "xmm18"); \
} while (0)

int main(void) {
    TEST_START("VINSERTPS EVEX");
    if (!CHECK_AVX512F()) return 0;
    const xmm_t base = {.u32 = {0x10, 0x20, 0x30, 0x40}};
    const xmm_t source = {.u32 = {0x111, 0x222, 0x333, 0x444}};
    xmm_t out;

    INSERT(0x00, out);
    TEST_ASSERT(out.u32[0] == source.u32[0], "VINSERTPS src0 dst0");
    TEST_ASSERT(out.u32[1] == base.u32[1] && out.u32[2] == base.u32[2] &&
                out.u32[3] == base.u32[3], "VINSERTPS preserves other lanes");

    INSERT(0x50, out);
    TEST_ASSERT(out.u32[1] == source.u32[1], "VINSERTPS src1 dst1");
    INSERT(0xa0, out);
    TEST_ASSERT(out.u32[2] == source.u32[2], "VINSERTPS src2 dst2");
    INSERT(0xf0, out);
    TEST_ASSERT(out.u32[3] == source.u32[3], "VINSERTPS src3 dst3");

    INSERT(0x99, out);
    TEST_ASSERT(out.u32[0] == 0 && out.u32[3] == 0,
                "VINSERTPS zero mask endpoints");
    TEST_ASSERT(out.u32[1] == source.u32[2] && out.u32[2] == base.u32[2],
                "VINSERTPS insert before zero mask");

    uint32_t memory = UINT32_C(0xa5a5a5a5);
    xmm_t mem0, mem3;
    __asm__ volatile(
        "vmovdqa32 %2, %%xmm16\n\t"
        "vinsertps $0x20, %3, %%xmm16, %%xmm17\n\tvmovdqa32 %%xmm17, %0\n\t"
        "vinsertps $0xe0, %3, %%xmm16, %%xmm18\n\tvmovdqa32 %%xmm18, %1"
        : "=m"(mem0), "=m"(mem3) : "m"(base), "m"(memory)
        : "xmm16", "xmm17", "xmm18");
    TEST_ASSERT(memcmp(&mem0, &mem3, sizeof(mem0)) == 0,
                "VINSERTPS memory ignores source-select bits");
    TEST_ASSERT(mem0.u32[2] == memory, "VINSERTPS memory destination lane");
    TEST_END();
}

#undef INSERT
