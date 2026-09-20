/*
 * Test VPACKSSDW with ymm registers (AVX2).
 * Pack signed dwords to signed words with saturation, independently in
 * each 128-bit lane.
 *
 * Compile: gcc -o test_vpackssdw avx256/test_vpackssdw.c -O0 -mavx2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include "../common.h"

#if ENABLE_RUNTIME_CPU_CHECKS
static int check_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    return (ebx >> 5) & 1;
}
#else
#define check_avx2() 1
#endif

static int16_t sat_i32_to_i16(int32_t value) {
    return value > INT16_MAX ? INT16_MAX :
           value < INT16_MIN ? INT16_MIN : (int16_t)value;
}

static void assert_result(const ymm_t *dst, const ymm_t *src1,
                          const ymm_t *src2, const char *form) {
    for (int lane = 0; lane < 2; lane++) {
        for (int element = 0; element < 4; element++) {
            TEST_ASSERT(dst->i16[lane * 8 + element] ==
                            sat_i32_to_i16(src1->i32[lane * 4 + element]),
                        "VPACKSSDW %s src1 lane%d element%d", form, lane, element);
            TEST_ASSERT(dst->i16[lane * 8 + 4 + element] ==
                            sat_i32_to_i16(src2->i32[lane * 4 + element]),
                        "VPACKSSDW %s src2 lane%d element%d", form, lane, element);
        }
    }
}

int main(void) {
    if (!check_avx2()) {
        printf("AVX2 not supported, skipping tests.\n");
        return 0;
    }
    TEST_START("VPACKSSDW (ymm)");

    ymm_t a = {.i32 = {
        INT32_MIN, -32769, -32768, -32767,
        -1, 0, 1, 32766
    }};
    ymm_t b = {.i32 = {
        32767, 32768, INT32_MAX, -40000,
        40000, 12345, -12345, 0
    }};
    ymm_t dst;

    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpackssdw %%ymm1, %%ymm0, %%ymm2\n\t"
        "vmovdqu %%ymm2, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1", "ymm2"
    );
    assert_result(&dst, &a, &b, "register");

    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vpackssdw %2, %%ymm0, %%ymm0\n\t"
        "vmovdqu %%ymm0, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0"
    );
    assert_result(&dst, &a, &b, "memory dst=src1");

    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vmovdqu %2, %%ymm1\n\t"
        "vpackssdw %%ymm1, %%ymm0, %%ymm1\n\t"
        "vmovdqu %%ymm1, %0"
        : "=m"(dst) : "m"(a), "m"(b) : "ymm0", "ymm1"
    );
    assert_result(&dst, &a, &b, "dst=src2");

    __asm__ volatile (
        "vmovdqu %1, %%ymm0\n\t"
        "vpackssdw %%ymm0, %%ymm0, %%ymm0\n\t"
        "vmovdqu %%ymm0, %0"
        : "=m"(dst) : "m"(a) : "ymm0"
    );
    assert_result(&dst, &a, &a, "dst=src1=src2");

    TEST_END();
}
