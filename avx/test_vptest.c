/* TARGET: VPTEST */
#include "../common.h"

#define RUN_VPTEST(width, av, bv, z, c) do { \
    __asm__ volatile("vmovdqu %2, %%" width "0\n\t" \
                     "vmovdqu %3, %%" width "1\n\t" \
                     "vptest %%" width "1, %%" width "0\n\t" \
                     "setz %0\n\tsetc %1" \
                     : "=qm"(z), "=qm"(c) : "m"(av), "m"(bv) \
                     : width "0", width "1", "cc"); \
} while (0)

static void run_cases(const char *width, int ymm) {
    ymm_t a, b;
    uint8_t zf, cf;
    (void)width;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (ymm) RUN_VPTEST("ymm", a, b, zf, cf);
    else RUN_VPTEST("xmm", a, b, zf, cf);
    TEST_ASSERT(zf == 1 && cf == 1, "VPTEST zero/zero width %d", ymm ? 256 : 128);

    memset(&a, 0xff, sizeof(a));
    for (int i = 0; i < 32; i++) b.u8[i] = (uint8_t)(i * 7 + 1);
    if (ymm) RUN_VPTEST("ymm", a, b, zf, cf);
    else RUN_VPTEST("xmm", a, b, zf, cf);
    TEST_ASSERT(zf == 0 && cf == 1, "VPTEST all-one containment width %d", ymm ? 256 : 128);

    memset(&a, 0, sizeof(a));
    memset(&b, 0xff, sizeof(b));
    if (ymm) RUN_VPTEST("ymm", a, b, zf, cf);
    else RUN_VPTEST("xmm", a, b, zf, cf);
    TEST_ASSERT(zf == 1 && cf == 0, "VPTEST disjoint width %d", ymm ? 256 : 128);
}

int main(void) {
    TEST_START("VPTEST");
    run_cases("xmm", 0);
    run_cases("ymm", 1);
    TEST_END();
}
