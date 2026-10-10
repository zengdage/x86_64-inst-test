/* VEX VPHMINPOSUW: unsigned minimum, first index, and destination zeroing. */
#include "../common.h"

static void check_result(const ymm_t *dst, unsigned value, unsigned index,
                         int case_index, const char *form) {
    for (int i = 0; i < 16; ++i) {
        unsigned expected = i == 0 ? value : i == 1 ? index : 0;
        TEST_ASSERT(dst->u16[i] == expected,
                    "vphminposuw %s case %d word %d: expected 0x%x, got 0x%x",
                    form, case_index, i, expected, dst->u16[i]);
    }
}

int main(void) {
    static const struct {
        xmm_t src;
        unsigned value, index;
    } cases[] = {
        {{.u16 = {0xffff, 0x8000, 0x7fff, 0x1234, 0x7fff, 1, 0x2222, 0x3333}}, 1, 5},
        {{.u16 = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}}, 0xffff, 0},
        {{.u16 = {0x8001, 0xffff, 0x8000, 0x8000, 0x9000, 0xa000, 0xb000, 0xc000}}, 0x8000, 2},
        {{.u16 = {7, 7, 7, 7, 1, 1, 9, 9}}, 1, 4},
        {{.u16 = {0, 1, 2, 3, 4, 5, 6, 7}}, 0, 0},
        {{.u16 = {7, 6, 5, 4, 3, 2, 1, 0}}, 0, 7},
    };

    TEST_START("VPHMINPOSUW");
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        ymm_t dst;
        __asm__ volatile (
            "vmovdqa %1, %%xmm0\n\t"
            "vpcmpeqd %%ymm1, %%ymm1, %%ymm1\n\t"
            "vphminposuw %%xmm0, %%xmm1\n\t"
            "vmovdqa %%ymm1, %0\n\t"
            "vzeroupper"
            : "=m"(dst) : "m"(cases[i].src) : "xmm0", "xmm1"
        );
        check_result(&dst, cases[i].value, cases[i].index, i, "register");

        __asm__ volatile (
            "vpcmpeqd %%ymm0, %%ymm0, %%ymm0\n\t"
            "vphminposuw %1, %%xmm0\n\t"
            "vmovdqa %%ymm0, %0\n\t"
            "vzeroupper"
            : "=m"(dst) : "m"(cases[i].src) : "xmm0"
        );
        check_result(&dst, cases[i].value, cases[i].index, i, "memory");

        __asm__ volatile (
            "vpcmpeqd %%ymm0, %%ymm0, %%ymm0\n\t"
            "vmovdqa %1, %%xmm0\n\t"
            "vphminposuw %%xmm0, %%xmm0\n\t"
            "vmovdqa %%ymm0, %0\n\t"
            "vzeroupper"
            : "=m"(dst) : "m"(cases[i].src) : "xmm0"
        );
        check_result(&dst, cases[i].value, cases[i].index, i, "aliased register");
    }
    TEST_END();
}
