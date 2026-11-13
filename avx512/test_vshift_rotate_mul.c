/* AVX-512 variable shifts, rotates, immediate count wrapping, and VPMULLQ. */
#include "avx512_test.h"

static uint32_t rol32(uint32_t value, unsigned count) {
    count &= 31U;
    return count ? (value << count) | (value >> (32U - count)) : value;
}

static uint32_t ror32(uint32_t value, unsigned count) {
    count &= 31U;
    return count ? (value >> count) | (value << (32U - count)) : value;
}

static void test_variable_dword(void) {
    zmm_test_t src, count, left, logical, arithmetic, rol, ror;
    const uint32_t counts[16] = {0, 1, 31, 32, 33, 63, 64, UINT32_MAX,
                                 2, 3, 4, 5, 6, 7, 8, 16};
    for (int i = 0; i < 16; i++) {
        src.u32[i] = UINT32_C(0x80000001) ^ (UINT32_C(0x11111111) * (uint32_t)i);
        count.u32[i] = counts[i];
    }
    __asm__ volatile(
        "vmovdqa32 %5, %%zmm0\n\tvmovdqa32 %6, %%zmm1\n\t"
        "vpsllvd %%zmm1, %%zmm0, %%zmm2\n\t"
        "vpsrlvd %%zmm1, %%zmm0, %%zmm3\n\t"
        "vpsravd %%zmm1, %%zmm0, %%zmm4\n\t"
        "vprolvd %%zmm1, %%zmm0, %%zmm5\n\t"
        "vprorvd %%zmm1, %%zmm0, %%zmm6\n\t"
        "vmovdqa32 %%zmm2, %0\n\tvmovdqa32 %%zmm3, %1\n\t"
        "vmovdqa32 %%zmm4, %2\n\tvmovdqa32 %%zmm5, %3\n\t"
        "vmovdqa32 %%zmm6, %4"
        : "=m"(left), "=m"(logical), "=m"(arithmetic), "=m"(rol), "=m"(ror)
        : "m"(src), "m"(count)
        : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5", "zmm6");
    for (int i = 0; i < 16; i++) {
        uint32_t c = count.u32[i];
        TEST_ASSERT(left.u32[i] == (c >= 32 ? 0U : src.u32[i] << c),
                    "VPSLLVD lane %d count %u", i, c);
        TEST_ASSERT(logical.u32[i] == (c >= 32 ? 0U : src.u32[i] >> c),
                    "VPSRLVD lane %d count %u", i, c);
        int32_t sar_expected = c >= 32 ? (src.i32[i] < 0 ? -1 : 0) : src.i32[i] >> c;
        TEST_ASSERT(arithmetic.i32[i] == sar_expected,
                    "VPSRAVD lane %d count %u", i, c);
        TEST_ASSERT(rol.u32[i] == rol32(src.u32[i], c), "VPROLVD lane %d", i);
        TEST_ASSERT(ror.u32[i] == ror32(src.u32[i], c), "VPRORVD lane %d", i);
    }
}

static void test_immediate_and_overlap(void) {
    zmm_test_t src, r0, r31, r32, r33;
    for (int i = 0; i < 16; i++) src.u32[i] = UINT32_C(0x01020304) * (uint32_t)(i + 1);
    __asm__ volatile(
        "vmovdqa32 %4, %%zmm0\n\tvprold $0, %%zmm0, %%zmm1\n\t"
        "vprold $31, %%zmm0, %%zmm2\n\tvprold $32, %%zmm0, %%zmm3\n\t"
        "vprold $33, %%zmm0, %%zmm0\n\t"
        "vmovdqa32 %%zmm1, %0\n\tvmovdqa32 %%zmm2, %1\n\t"
        "vmovdqa32 %%zmm3, %2\n\tvmovdqa32 %%zmm0, %3"
        : "=m"(r0), "=m"(r31), "=m"(r32), "=m"(r33) : "m"(src)
        : "zmm0", "zmm1", "zmm2", "zmm3");
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT(r0.u32[i] == src.u32[i], "VPROLD count 0 lane %d", i);
        TEST_ASSERT(r31.u32[i] == rol32(src.u32[i], 31), "VPROLD count 31 lane %d", i);
        TEST_ASSERT(r32.u32[i] == src.u32[i], "VPROLD count 32 wraps lane %d", i);
        TEST_ASSERT(r33.u32[i] == rol32(src.u32[i], 1),
                    "VPROLD overlapping destination count 33 lane %d", i);
    }
}

static void test_qword_multiply(void) {
    zmm_test_t a, b, out;
    const uint64_t av[8] = {0, 1, UINT64_MAX, UINT64_C(0x8000000000000000),
                            UINT64_C(0x100000001), 7, 11, UINT64_C(0xdeadbeef)};
    const uint64_t bv[8] = {UINT64_MAX, 2, UINT64_MAX, 2,
                            UINT64_C(0xffffffff), UINT64_MAX, 13, UINT64_C(0x12345678)};
    memcpy(a.u64, av, sizeof(av));
    memcpy(b.u64, bv, sizeof(bv));
    __asm__ volatile(
        "vmovdqa64 %1, %%zmm0\n\tvmovdqa64 %2, %%zmm1\n\t"
        "vpmullq %%zmm1, %%zmm0, %%zmm0\n\tvmovdqa64 %%zmm0, %0"
        : "=m"(out) : "m"(a), "m"(b) : "zmm0", "zmm1");
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(out.u64[i] == a.u64[i] * b.u64[i],
                    "VPMULLQ low product and overlap lane %d", i);
}

int main(void) {
    TEST_START("AVX-512 variable shifts, rotates, and VPMULLQ");
    if (!CHECK_AVX512F()) return 0;
    test_variable_dword();
    test_immediate_and_overlap();
    test_qword_multiply();
    TEST_END();
}
