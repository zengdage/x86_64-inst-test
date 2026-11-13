/* AVX-512 narrowing, saturating conversion, extension, and mask move forms. */
#include "avx512_test.h"
#include <limits.h>

static int64_t clamp_signed(int64_t value, int bits) {
    int64_t lo = -(INT64_C(1) << (bits - 1));
    int64_t hi = (INT64_C(1) << (bits - 1)) - 1;
    return value < lo ? lo : value > hi ? hi : value;
}

static uint64_t clamp_unsigned(uint64_t value, int bits) {
    uint64_t hi = (UINT64_C(1) << bits) - 1;
    return value > hi ? hi : value;
}

static void test_truncating_narrow(void) {
    zmm_test_t src = {0}, out = {0};
    for (int i = 0; i < 8; i++)
        src.u64[i] = UINT64_C(0x1122334455667788) + UINT64_C(0x101010101010101) * (uint64_t)i;

#define TEST_Q_NARROW(INST, STORE, FIELD, LANES, TYPE, LABEL) do { \
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\t" #INST " %%zmm0, %%xmm1\n\t" \
                     #STORE " %%xmm1, %0" \
                     : "=m"(out) : "m"(src) : "zmm0", "xmm1"); \
    for (int i = 0; i < (LANES); i++) \
        TEST_ASSERT(out.FIELD[i] == (TYPE)src.u64[i], LABEL " lane %d", i); \
} while (0)
    TEST_Q_NARROW(vpmovqb, vmovq, u8, 8, uint8_t, "VPMOVQB");
    TEST_Q_NARROW(vpmovqw, vmovdqu, u16, 8, uint16_t, "VPMOVQW");
#undef TEST_Q_NARROW

    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvpmovqd %%zmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "ymm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u32[i] == (uint32_t)src.u64[i], "VPMOVQD lane %d", i);

    for (int i = 0; i < 32; i++) src.u16[i] = (uint16_t)(0x1200U + i * 257U);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvpmovwb %%zmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "ymm1");
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(out.u8[i] == (uint8_t)src.u16[i], "VPMOVWB lane %d", i);
}

static void test_signed_saturation(void) {
    zmm_test_t src = {0}, bytes = {0}, words = {0}, out = {0};
    const int32_t dwords[16] = {
        INT32_MIN, -65537, -65536, -32769, -32768, -129, -128, -1,
        0, 1, 127, 128, 255, 32767, 32768, INT32_MAX
    };
    memcpy(src.i32, dwords, sizeof(dwords));
    __asm__ volatile("vmovdqa64 %2, %%zmm0\n\tvpmovsdb %%zmm0, %%xmm1\n\t"
                     "vpmovsdw %%zmm0, %%ymm2\n\tvmovdqu %%xmm1, %0\n\tvmovdqu %%ymm2, %1"
                     : "=m"(bytes), "=m"(words) : "m"(src)
                     : "zmm0", "xmm1", "ymm2");
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(bytes.i8[i] == clamp_signed(src.i32[i], 8), "VPMOVSDB lane %d", i);
        TEST_ASSERT(words.i16[i] == clamp_signed(src.i32[i], 16), "VPMOVSDW lane %d", i);
    }

    const int64_t qwords[8] = {
        INT64_MIN, -32769, -129, -1, 0, 127, 32767, INT64_MAX
    };
    memcpy(src.i64, qwords, sizeof(qwords));
    zmm_test_t dword = {0}, word = {0};
    __asm__ volatile("vmovdqa64 %3, %%zmm0\n\tvpmovsqb %%zmm0, %%xmm1\n\t"
                     "vpmovsqd %%zmm0, %%ymm2\n\tvpmovsqw %%zmm0, %%xmm3\n\t"
                     "vmovq %%xmm1, %0\n\tvmovdqu %%ymm2, %1\n\tvmovdqu %%xmm3, %2"
                     : "=m"(bytes), "=m"(dword), "=m"(word) : "m"(src)
                     : "zmm0", "xmm1", "ymm2", "xmm3");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(bytes.i8[i] == clamp_signed(src.i64[i], 8), "VPMOVSQB lane %d", i);
        TEST_ASSERT(dword.i32[i] == clamp_signed(src.i64[i], 32), "VPMOVSQD lane %d", i);
        TEST_ASSERT(word.i16[i] == clamp_signed(src.i64[i], 16), "VPMOVSQW lane %d", i);
    }

    for (int i = 0; i < 32; i++) src.i16[i] = (int16_t)((i - 16) * 1000);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvpmovswb %%zmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(out) : "m"(src) : "zmm0", "ymm1");
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(out.i8[i] == clamp_signed(src.i16[i], 8), "VPMOVSWB lane %d", i);
}

static void test_unsigned_saturation(void) {
    zmm_test_t src = {0}, bytes = {0}, words = {0};
    const int32_t dwords[16] = {
        INT32_MIN, -1, 0, 1, 127, 128, 255, 256,
        32767, 32768, 65535, 65536, 100000, -1000, INT32_MAX, 42
    };
    memcpy(src.i32, dwords, sizeof(dwords));
    __asm__ volatile("vmovdqa64 %2, %%zmm0\n\tvpmovusdb %%zmm0, %%xmm1\n\t"
                     "vpmovusdw %%zmm0, %%ymm2\n\tvmovdqu %%xmm1, %0\n\tvmovdqu %%ymm2, %1"
                     : "=m"(bytes), "=m"(words) : "m"(src)
                     : "zmm0", "xmm1", "ymm2");
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(bytes.u8[i] == clamp_unsigned(src.u32[i], 8), "VPMOVUSDB lane %d", i);
        TEST_ASSERT(words.u16[i] == clamp_unsigned(src.u32[i], 16), "VPMOVUSDW lane %d", i);
    }

    const int64_t qwords[8] = {INT64_MIN, -1, 0, 255, 256, 65535, 65536, INT64_MAX};
    memcpy(src.i64, qwords, sizeof(qwords));
    zmm_test_t dword = {0};
    __asm__ volatile("vmovdqa64 %3, %%zmm0\n\tvpmovusqb %%zmm0, %%xmm1\n\t"
                     "vpmovusqd %%zmm0, %%ymm2\n\tvpmovusqw %%zmm0, %%xmm3\n\t"
                     "vmovq %%xmm1, %0\n\tvmovdqu %%ymm2, %1\n\tvmovdqu %%xmm3, %2"
                     : "=m"(bytes), "=m"(dword), "=m"(words) : "m"(src)
                     : "zmm0", "xmm1", "ymm2", "xmm3");
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT(bytes.u8[i] == clamp_unsigned(src.u64[i], 8), "VPMOVUSQB lane %d", i);
        TEST_ASSERT(dword.u32[i] == clamp_unsigned(src.u64[i], 32), "VPMOVUSQD lane %d", i);
        TEST_ASSERT(words.u16[i] == clamp_unsigned(src.u64[i], 16), "VPMOVUSQW lane %d", i);
    }

    for (int i = 0; i < 32; i++) src.i16[i] = (int16_t)((i - 8) * 100);
    __asm__ volatile("vmovdqa64 %1, %%zmm0\n\tvpmovuswb %%zmm0, %%ymm1\n\t"
                     "vmovdqu %%ymm1, %0"
                     : "=m"(bytes) : "m"(src) : "zmm0", "ymm1");
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(bytes.u8[i] == clamp_unsigned(src.u16[i], 8), "VPMOVUSWB lane %d", i);
}

static void test_extend_and_masks(void) {
    ymm_t src;
    zmm_test_t signed_out, unsigned_out, mask_out;
    for (int i = 0; i < 32; i++) src.i8[i] = (int8_t)(i * 9 - 120);
    __asm__ volatile("vpmovsxbw %2, %%zmm0\n\tvpmovzxbw %2, %%zmm1\n\t"
                     "vmovdqa64 %%zmm0, %0\n\tvmovdqa64 %%zmm1, %1"
                     : "=m"(signed_out), "=m"(unsigned_out) : "m"(src)
                     : "zmm0", "zmm1");
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT(signed_out.i16[i] == src.i8[i], "VPMOVSXBW lane %d", i);
        TEST_ASSERT(unsigned_out.u16[i] == (uint8_t)src.u8[i], "VPMOVZXBW lane %d", i);
    }

    uint32_t mask = UINT32_C(0xa55aa55a), round_trip;
    __asm__ volatile("kmovd %2, %%k1\n\tvpmovm2w %%k1, %%zmm0\n\t"
                     "vpmovw2m %%zmm0, %%k2\n\tkmovd %%k2, %1\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(mask_out), "=r"(round_trip) : "r"(mask)
                     : "k1", "k2", "zmm0");
    for (int i = 0; i < 32; i++)
        TEST_ASSERT(mask_out.u16[i] == ((mask >> i) & 1U ? UINT16_MAX : 0),
                    "VPMOVM2W lane %d", i);
    TEST_ASSERT(round_trip == mask, "VPMOVW2M round trip");
}

int main(void) {
    TEST_START("AVX-512 narrowing and mask move forms");
    if (!CHECK_AVX512F()) return 0;
    test_truncating_narrow();
    test_signed_saturation();
    test_unsigned_saturation();
    test_extend_and_masks();
    TEST_END();
}
