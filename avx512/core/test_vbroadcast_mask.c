/* EVEX broadcasts and vector-sign-bit/opmask conversions. */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

static void test_integer_broadcasts(void) {
    zmm_test_t out;
    uint32_t dword = UINT32_C(0x89abcdef);
    uint64_t qword = UINT64_C(0x0123456789abcdef);
    uint32_t byte = 0xa5U;
    uint32_t word = 0xbeefU;
    __asm__ volatile("vpbroadcastb %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "r"(byte) : "zmm0");
    for (int i = 0; i < 64; i++) TEST_ASSERT(out.u8[i] == (uint8_t)byte, "VPBROADCASTB lane %d", i);
    __asm__ volatile("vpbroadcastw %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "r"(word) : "zmm0");
    for (int i = 0; i < 32; i++) TEST_ASSERT(out.u16[i] == (uint16_t)word, "VPBROADCASTW lane %d", i);
    __asm__ volatile("vpbroadcastd %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "r"(dword) : "zmm0");
    for (int i = 0; i < 16; i++) TEST_ASSERT(out.u32[i] == dword, "VPBROADCASTD lane %d", i);
    __asm__ volatile("vpbroadcastq %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "r"(qword) : "zmm0");
    for (int i = 0; i < 8; i++) TEST_ASSERT(out.u64[i] == qword, "VPBROADCASTQ lane %d", i);
}

static void test_float_and_block_broadcasts(void) {
    zmm_test_t src = {0}, out;
    src.u32[0] = UINT32_C(0x7fc12345);
    __asm__ volatile(
        "vmovdqu32 %1, %%xmm0\n\tvbroadcastss %%xmm0, %%zmm1\n\t"
        "vmovdqu32 %%zmm1, %0"
        : "=m"(out) : "m"(src) : "xmm0", "zmm1");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == src.u32[0], "VBROADCASTSS preserves QNaN bits lane %d", i);

    for (int i = 0; i < 4; i++) src.u32[i] = UINT32_C(0x10000000) + (uint32_t)i;
    __asm__ volatile("vbroadcastf32x4 %1, %%zmm0\n\tvmovdqa32 %%zmm0, %0"
                     : "=m"(out) : "m"(src) : "zmm0");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.u32[i] == src.u32[i & 3], "VBROADCASTF32X4 lane %d", i);

    for (int i = 0; i < 4; i++) src.u64[i] = UINT64_C(0x2000000000000000) + (uint64_t)i;
    __asm__ volatile("vbroadcastf64x4 %1, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
                     : "=m"(out) : "m"(src) : "zmm0");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == src.u64[i & 3], "VBROADCASTF64X4 lane %d", i);
}

static void test_mask_conversions(void) {
    zmm_test_t src, out;
    for (int i = 0; i < 16; i++) src.u32[i] = (i == 0 || i == 7 || i == 15) ? UINT32_C(0x80000000) : (uint32_t)i;
    uint32_t mask;
    __asm__ volatile(
        "vmovdqa32 %1, %%zmm0\n\tvpmovd2m %%zmm0, %%k1\n\t"
        "kmovw %%k1, %0\n\tvpmovm2d %%k1, %%zmm1\n\tvmovdqa32 %%zmm1, %2"
        : "=r"(mask), "+m"(src), "=m"(out) : : "zmm0", "zmm1", "k1");
    TEST_ASSERT((mask & 0xffffU) == 0x8081U, "VPMOVD2M sign-bit mask 0x%04x", mask);
    for (int i = 0; i < 16; i++) {
        uint32_t expected = ((mask >> i) & 1U) ? UINT32_MAX : 0;
        TEST_ASSERT(out.u32[i] == expected, "VPMOVM2D lane %d", i);
    }

    uint64_t expected_b = 0;
    for (int i = 0; i < 64; i++) {
        src.u8[i] = (i % 9 == 0 || i == 63) ? 0x80U : (uint8_t)i;
        expected_b |= (uint64_t)(src.u8[i] >> 7) << i;
    }
    uint64_t mask64;
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvpmovb2m %%zmm0, %%k1\n\t"
        "kmovq %%k1, %0\n\tvpmovm2b %%k1, %%zmm1\n\tvmovdqa64 %%zmm1, %2"
        : "=r"(mask64), "+m"(src), "=m"(out) : : "zmm0", "zmm1", "k1");
    TEST_ASSERT(mask64 == expected_b, "VPMOVB2M 64-bit mask");
    for (int i = 0; i < 64; i++)
        TEST_ASSERT(out.u8[i] == (((mask64 >> i) & 1U) ? 0xffU : 0),
                    "VPMOVM2B lane %d", i);

    uint32_t qmask = 0xa5U;
    __asm__ volatile(
        "kmovb %2, %%k1\n\tvpmovm2q %%k1, %%zmm0\n\t"
        "vpmovq2m %%zmm0, %%k2\n\tkmovb %%k2, %0\n\tvmovdqa64 %%zmm0, %1"
        : "=r"(mask), "=m"(out) : "r"(qmask) : "zmm0", "k1", "k2");
    TEST_ASSERT((mask & 0xffU) == qmask, "VPMOVM2Q/VPMOVQ2M round trip");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == (((qmask >> i) & 1U) ? UINT64_MAX : 0),
                    "VPMOVM2Q lane %d", i);
}

static void test_masked_memory_fault_suppression(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "broadcast guard page allocation");
    if (guard == MAP_FAILED) return;
    zmm_test_t out;
    uint32_t zero = 0;
    __asm__ volatile(
        "kmovw %2, %%k1\n\tvpbroadcastd (%1), %%zmm0%{%%k1%}%{z%}\n\t"
        "vmovdqa32 %%zmm0, %0"
        : "=m"(out) : "r"(guard), "r"(zero) : "zmm0", "k1", "memory");
    for (int i = 0; i < 16; i++) TEST_ASSERT(out.u32[i] == 0, "masked broadcast zero lane %d", i);
    munmap(guard, (size_t)page_size);
}

int main(void) {
    TEST_START("AVX-512 broadcasts and mask/vector conversion");
    if (!CHECK_AVX512F()) return 0;
    test_integer_broadcasts();
    test_float_and_block_broadcasts();
    test_mask_conversions();
    test_masked_memory_fault_suppression();
    TEST_END();
}
