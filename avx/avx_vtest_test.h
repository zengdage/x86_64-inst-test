#ifndef X86_64_AVX_VTEST_TEST_H
#define X86_64_AVX_VTEST_TEST_H

#include "../common.h"

#define RUN_TEST(width, av, bv, z, c) do { \
    __asm__ volatile("vmovdqu %2, %%" width "0\n\tvmovdqu %3, %%" width "1\n\t" \
                     TEST_INSN " %%" width "1, %%" width "0\n\t" \
                     "setz %0\n\tsetc %1" \
                     : "=qm"(z), "=qm"(c) : "m"(av), "m"(bv) \
                     : width "0", width "1", "cc"); \
} while (0)

int main(void) {
    ymm_t a, b;
    uint8_t zf, cf;
    TEST_START(TEST_NAME);
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    RUN_TEST("xmm", a, b, zf, cf);
    TEST_ASSERT(zf && cf, TEST_NAME " zero/zero xmm");
    RUN_TEST("ymm", a, b, zf, cf);
    TEST_ASSERT(zf && cf, TEST_NAME " zero/zero ymm");
    memset(&a, 0xff, sizeof(a));
    for (int i = 0; i < 32; i++) b.u8[i] = (uint8_t)(i * 9 + 1);
    RUN_TEST("ymm", a, b, zf, cf);
    TEST_ASSERT(!zf && cf, TEST_NAME " contained bits");
    memset(&a, 0, sizeof(a));
    memset(&b, 0xff, sizeof(b));
    RUN_TEST("ymm", a, b, zf, cf);
    TEST_ASSERT(zf && !cf, TEST_NAME " disjoint bits");
    TEST_END();
}

#endif
