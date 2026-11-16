#ifndef X86_64_AVX_SHIFT_IMM_TEST_H
#define X86_64_AVX_SHIFT_IMM_TEST_H

#include "../common.h"

#define SHIFT_LEFT 1
#define SHIFT_RIGHT 2
#define SHIFT_ARITH 3
#define SHIFT_BYTES_LEFT 4
#define SHIFT_BYTES_RIGHT 5

#if TEST_BITS == 16
#define SHIFT_FIELD u16
#define SHIFT_LANES 16
#define SHIFT_SIGN UINT16_C(0x8000)
#define SHIFT_ALL UINT16_MAX
#elif TEST_BITS == 32
#define SHIFT_FIELD u32
#define SHIFT_LANES 8
#define SHIFT_SIGN UINT32_C(0x80000000)
#define SHIFT_ALL UINT32_MAX
#elif TEST_BITS == 64
#define SHIFT_FIELD u64
#define SHIFT_LANES 4
#define SHIFT_SIGN UINT64_C(0x8000000000000000)
#define SHIFT_ALL UINT64_MAX
#else
#define SHIFT_LANES 32
#endif

#if TEST_SHIFT != SHIFT_BYTES_LEFT && TEST_SHIFT != SHIFT_BYTES_RIGHT
static __typeof__(((ymm_t *)0)->SHIFT_FIELD[0]) expected_element(
    __typeof__(((ymm_t *)0)->SHIFT_FIELD[0]) value, unsigned count) {
    if (count >= TEST_BITS) {
#if TEST_SHIFT == SHIFT_ARITH
        return value & SHIFT_SIGN ? SHIFT_ALL : 0;
#else
        return 0;
#endif
    }
#if TEST_SHIFT == SHIFT_LEFT
    return (__typeof__(value))(value << count);
#elif TEST_SHIFT == SHIFT_RIGHT
    return (__typeof__(value))(value >> count);
#else
    if (count == 0) return value;
    return (__typeof__(value))((value >> count) |
           ((value & SHIFT_SIGN) ? (SHIFT_ALL << (TEST_BITS - count)) : 0));
#endif
}
#endif

static void check_result(const ymm_t *out, const ymm_t *src, unsigned count) {
#if TEST_SHIFT == SHIFT_BYTES_LEFT || TEST_SHIFT == SHIFT_BYTES_RIGHT
    for (int i = 0; i < 32; i++) {
        int lane_base = (i / 16) * 16;
        int source = TEST_SHIFT == SHIFT_BYTES_LEFT ? i - (int)count : i + (int)count;
        uint8_t expected = source >= lane_base && source < lane_base + 16 ? src->u8[source] : 0;
        TEST_ASSERT(out->u8[i] == expected, TEST_NAME " count %u byte %d", count, i);
    }
#else
    for (int i = 0; i < SHIFT_LANES; i++)
        TEST_ASSERT(out->SHIFT_FIELD[i] == expected_element(src->SHIFT_FIELD[i], count),
                    TEST_NAME " count %u lane %d", count, i);
#endif
}

#define RUN_SHIFT(imm, src, out) \
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" \
                     TEST_INSN " $" #imm ", %%ymm0, %%ymm1\n\t" \
                     "vmovdqu %%ymm1, %0" \
                     : "=m"(out) : "m"(src) : "ymm0", "ymm1")

int main(void) {
    ymm_t src, out;
    TEST_START(TEST_NAME);
    for (int i = 0; i < 32; i++) src.u8[i] = (uint8_t)(0x80U + (unsigned)i);
    RUN_SHIFT(0, src, out); check_result(&out, &src, 0);
#if TEST_SHIFT == SHIFT_BYTES_LEFT || TEST_SHIFT == SHIFT_BYTES_RIGHT
    RUN_SHIFT(15, src, out); check_result(&out, &src, 15);
    RUN_SHIFT(16, src, out); check_result(&out, &src, 16);
    RUN_SHIFT(17, src, out); check_result(&out, &src, 17);
#else
#if TEST_BITS == 16
    RUN_SHIFT(15, src, out); check_result(&out, &src, 15);
    RUN_SHIFT(16, src, out); check_result(&out, &src, 16);
    RUN_SHIFT(17, src, out); check_result(&out, &src, 17);
#elif TEST_BITS == 32
    RUN_SHIFT(31, src, out); check_result(&out, &src, 31);
    RUN_SHIFT(32, src, out); check_result(&out, &src, 32);
    RUN_SHIFT(33, src, out); check_result(&out, &src, 33);
#else
    RUN_SHIFT(63, src, out); check_result(&out, &src, 63);
    RUN_SHIFT(64, src, out); check_result(&out, &src, 64);
    RUN_SHIFT(65, src, out); check_result(&out, &src, 65);
#endif
#endif
    RUN_SHIFT(255, src, out); check_result(&out, &src, 255);
    TEST_END();
}

#endif
