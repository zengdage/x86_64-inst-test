/* TARGET: VMOVDDUP */
#include "../common.h"

int main(void) {
    TEST_START("VMOVDDUP");
    const ymm_t src = {.u64 = {
        UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210),
        UINT64_C(0x1111222233334444), UINT64_C(0xaaaabbbbccccdddd)
    }};
    ymm_t out;
    __asm__ volatile("vmovddup %1, %%ymm0\n\tvmovdqa %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0");
    TEST_ASSERT(out.u64[0] == src.u64[0], "VMOVDDUP lane 0");
    TEST_ASSERT(out.u64[1] == src.u64[0], "VMOVDDUP lane 1");
    TEST_ASSERT(out.u64[2] == src.u64[2], "VMOVDDUP lane 2");
    TEST_ASSERT(out.u64[3] == src.u64[2], "VMOVDDUP lane 3");
    TEST_END();
}
