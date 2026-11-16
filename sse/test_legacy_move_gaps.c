/* TARGETS: MOVHLPS MOVLHPS LDDQU MOVD */
#include "../common.h"

static void test_legacy_moves(void) {
    const xmm_t a = { .u64 = {
        UINT64_C(0x1111222233334444), UINT64_C(0x5555666677778888)
    } };
    const xmm_t b = { .u64 = {
        UINT64_C(0x9999aaaabbbbcccc), UINT64_C(0xddddeeeeffff0000)
    } };
    xmm_t out;

    __asm__ volatile("movdqu %1, %%xmm0\n\tmovdqu %2, %%xmm1\n\t"
                     "movhlps %%xmm1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm1");
    TEST_ASSERT(out.u64[0] == b.u64[1], "MOVHLPS copies source high qword to low");
    TEST_ASSERT(out.u64[1] == a.u64[1], "MOVHLPS preserves destination high qword");

    __asm__ volatile("movdqu %1, %%xmm0\n\tmovdqu %2, %%xmm1\n\t"
                     "movlhps %%xmm1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "xmm0", "xmm1");
    TEST_ASSERT(out.u64[0] == a.u64[0], "MOVLHPS preserves destination low qword");
    TEST_ASSERT(out.u64[1] == b.u64[0], "MOVLHPS copies source low qword");

    struct {
        uint8_t prefix;
        uint8_t data[16];
    } __attribute__((packed)) unaligned;
    unaligned.prefix = 0xa5;
    for (int i = 0; i < 16; i++) unaligned.data[i] = (uint8_t)(i * 13 + 7);
    __asm__ volatile("lddqu %1, %%xmm0\n\tmovdqu %%xmm0, %0"
                     : "=m"(out) : "m"(unaligned.data) : "xmm0");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u8[i] == unaligned.data[i], "LDDQU unaligned byte %d", i);

    const uint32_t value = UINT32_C(0x89abcdef);
    __asm__ volatile("pcmpeqd %%xmm0, %%xmm0\n\tmovd %1, %%xmm0\n\t"
                     "movdqu %%xmm0, %0"
                     : "=m"(out) : "r"(value) : "xmm0");
    TEST_ASSERT(out.u32[0] == value, "MOVD GPR to XMM low dword");
    TEST_ASSERT(out.u32[1] == 0 && out.u32[2] == 0 && out.u32[3] == 0,
                "MOVD GPR to XMM clears high bits");
    uint64_t gpr = UINT64_MAX;
    __asm__ volatile("movd %1, %k0" : "+r"(gpr) : "x"(out));
    TEST_ASSERT(gpr == value, "MOVD XMM to GPR zero extends");
}

int main(void) {
    TEST_START("Legacy SSE move instruction gaps");
    test_legacy_moves();
    TEST_END();
}
