/* TARGET: VPSLLVQ */
#include "../common.h"

static void run_case(const ymm_t *src, const ymm_t *counts) {
    ymm_t out;
    __asm__ volatile("vmovdqa %1, %%ymm0\n\tvmovdqa %2, %%ymm1\n\t"
                     "vpsllvq %%ymm1, %%ymm0, %%ymm2\n\tvmovdqa %%ymm2, %0"
                     : "=m"(out) : "m"(*src), "m"(*counts)
                     : "ymm0", "ymm1", "ymm2");
    for (int i = 0; i < 4; i++) {
        uint64_t expected = counts->u64[i] >= 64 ? 0 : src->u64[i] << counts->u64[i];
        TEST_ASSERT(out.u64[i] == expected, "VPSLLVQ lane %d count %llu", i,
                    (unsigned long long)counts->u64[i]);
    }
}

int main(void) {
    TEST_START("VPSLLVQ");
    const ymm_t src = {.u64 = {
        UINT64_C(1), UINT64_C(3), UINT64_C(0x8000000000000001), UINT64_MAX
    }};
    const ymm_t boundary = {.u64 = {0, 63, 64, 65}};
    const ymm_t high = {.u64 = {1, UINT64_MAX, 4, UINT64_C(0x8000000000000000)}};
    run_case(&src, &boundary);
    run_case(&src, &high);
    TEST_END();
}
