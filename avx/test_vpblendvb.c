/* TARGET: VPBLENDVB */
#include "../common.h"

static void check_bytes(const ymm_t *out, const ymm_t *a, const ymm_t *b,
                        const ymm_t *mask, int count) {
    for (int i = 0; i < count; i++) {
        uint8_t expected = mask->u8[i] & 0x80 ? b->u8[i] : a->u8[i];
        TEST_ASSERT(out->u8[i] == expected,
                    "VPBLENDVB lane %d mask %#x", i, mask->u8[i]);
    }
}

int main(void) {
    ymm_t a, b, mask, out, initial;
    TEST_START("VPBLENDVB");
    for (int i = 0; i < 32; i++) {
        a.u8[i] = (uint8_t)i;
        b.u8[i] = (uint8_t)(0xe0U - (unsigned)i);
        mask.u8[i] = (uint8_t)((i % 5 == 0 || i == 31) ? 0x80 :
                               (i % 3 == 0 ? 0xff : 0x7f));
    }
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t"
                     "vmovdqu %3, %%ymm2\n\t"
                     "vpblendvb %%ymm2, %%ymm1, %%ymm0, %%ymm3\n\t"
                     "vmovdqu %%ymm3, %0"
                     : "=m"(out) : "m"(a), "m"(b), "m"(mask)
                     : "ymm0", "ymm1", "ymm2", "ymm3");
    check_bytes(&out, &a, &b, &mask, 32);

    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm3\n\tmovdqu %2, %%xmm0\n\t"
                     "movdqu %3, %%xmm1\n\tmovdqu %4, %%xmm2\n\t"
                     "vpblendvb %%xmm2, %%xmm1, %%xmm0, %%xmm3\n\t"
                     "vmovdqu %%ymm3, %0"
                     : "=m"(out)
                     : "m"(initial), "m"(a), "m"(b), "m"(mask)
                     : "xmm0", "xmm1", "xmm2", "ymm3");
    check_bytes(&out, &a, &b, &mask, 16);
    for (int i = 16; i < 32; i++)
        TEST_ASSERT(out.u8[i] == 0, "VPBLENDVB VEX.128 upper byte %d", i);
    TEST_END();
}
