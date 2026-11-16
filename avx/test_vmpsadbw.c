/* TARGET: VMPSADBW */
#include "../common.h"

static unsigned sad4(const uint8_t *a, const uint8_t *b) {
    unsigned sum = 0;
    for (int i = 0; i < 4; i++) {
        int delta = (int)a[i] - (int)b[i];
        sum += (unsigned)(delta < 0 ? -delta : delta);
    }
    return sum;
}

static void check_lane(const ymm_t *out, const ymm_t *a, const ymm_t *b,
                       unsigned imm, int half) {
    int a_base = half * 16 + (int)(((imm >> (half ? 5 : 2)) & 1U) * 4U);
    int b_base = half * 16 + (int)(((imm >> (half ? 3 : 0)) & 3U) * 4U);
    for (int lane = 0; lane < 8; lane++)
        TEST_ASSERT(out->u16[half * 8 + lane] ==
                    sad4(&a->u8[a_base + lane], &b->u8[b_base]),
                    "VMPSADBW imm %#x half %d lane %d", imm, half, lane);
}

#define RUN_MPSAD(imm, a, b, out) \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\tvmovdqu %2, %%ymm1\n\t" \
                     "vmpsadbw $" #imm ", %%ymm1, %%ymm0, %%ymm2\n\t" \
                     "vmovdqu %%ymm2, %0" \
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2")

int main(void) {
    ymm_t a, b, out;
    TEST_START("VMPSADBW");
    for (int i = 0; i < 32; i++) {
        a.u8[i] = (uint8_t)(i * 7 + 3);
        b.u8[i] = (uint8_t)(255 - i * 5);
    }
    RUN_MPSAD(0, a, b, out);
    check_lane(&out, &a, &b, 0, 0);
    check_lane(&out, &a, &b, 0, 1);
    RUN_MPSAD(63, a, b, out);
    check_lane(&out, &a, &b, 63, 0);
    check_lane(&out, &a, &b, 63, 1);
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t"
                     "vmpsadbw $0x15, %2, %%ymm0, %%ymm2\n\t"
                     "vmovdqu %%ymm2, %0"
                     : "=m"(out) : "m"(a), "m"(b) : "ymm0", "ymm2");
    check_lane(&out, &a, &b, 0x15, 0);
    check_lane(&out, &a, &b, 0x15, 1);
    TEST_END();
}
