/* TARGETS: VMOVHLPS VMOVLHPS VMOVD */
#include "../common.h"

int main(void) {
    const xmm_t a = { .u64 = {
        UINT64_C(0x0123456789abcdef), UINT64_C(0x1111111122222222)
    } };
    const xmm_t b = { .u64 = {
        UINT64_C(0xfedcba9876543210), UINT64_C(0x3333333344444444)
    } };
    ymm_t out;

    TEST_START("VEX move instruction gaps");
    __asm__ volatile("vmovhlps %2, %1, %%xmm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "x"(a), "x"(b) : "xmm0");
    TEST_ASSERT(out.u64[0] == b.u64[1], "VMOVHLPS source2 high qword to low");
    TEST_ASSERT(out.u64[1] == a.u64[1], "VMOVHLPS source1 high qword");
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0, "VMOVHLPS clears upper YMM");

    __asm__ volatile("vmovlhps %2, %1, %%xmm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "x"(a), "x"(b) : "xmm0");
    TEST_ASSERT(out.u64[0] == a.u64[0], "VMOVLHPS source1 low qword");
    TEST_ASSERT(out.u64[1] == b.u64[0], "VMOVLHPS source2 low qword");
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0, "VMOVLHPS clears upper YMM");

    const uint32_t value = UINT32_C(0x80000001);
    memset(&out, 0xa5, sizeof(out));
    __asm__ volatile("vmovd %1, %%xmm0\n\tvmovdqu %%ymm0, %0"
                     : "=m"(out) : "r"(value) : "xmm0");
    TEST_ASSERT(out.u32[0] == value, "VMOVD GPR to XMM low dword");
    for (int i = 1; i < 8; i++)
        TEST_ASSERT(out.u32[i] == 0, "VMOVD clears destination lane %d", i);

    __asm__ volatile("vzeroupper");
    TEST_END();
}
