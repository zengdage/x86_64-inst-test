/* TARGET: VINSERTF128 */
#include "../common.h"

#define RUN_INSERT(imm, base, lane, out) \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" \
                     "vinsertf128 $" #imm ", %2, %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0" \
                     : "=m"(out) : "m"(base), "m"(lane) : "ymm0", "ymm1")

static void check(const ymm_t *out, const ymm_t *base, const xmm_t *lane,
                  int high) {
    for (int i = 0; i < 8; i++) {
        uint32_t expected = ((i >= 4) == high) ? lane->u32[i & 3] : base->u32[i];
        TEST_ASSERT(out->u32[i] == expected,
                    "VINSERTF128 %s lane %d", high ? "high" : "low", i);
    }
}

int main(void) {
    ymm_t base, out;
    xmm_t lane;
    TEST_START("VINSERTF128");
    for (int i = 0; i < 8; i++) base.u32[i] = UINT32_C(0x10000000) + (uint32_t)i;
    for (int i = 0; i < 4; i++) lane.u32[i] = UINT32_C(0xdead0000) + (uint32_t)i;
    RUN_INSERT(0, base, lane, out);
    check(&out, &base, &lane, 0);
    RUN_INSERT(1, base, lane, out);
    check(&out, &base, &lane, 1);
    RUN_INSERT(255, base, lane, out);
    check(&out, &base, &lane, 1);
    TEST_END();
}
