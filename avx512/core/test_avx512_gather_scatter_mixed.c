/* AVX-512 mixed-width gather/scatter forms: DQ uses dword indices and qword
 * elements, while QD uses qword indices and dword elements.  The floating
 * DPD/QPS opcodes use the same addressing shapes but have distinct encodings.
 */
#include "../include/avx512_test.h"
#include <limits.h>
#include <sys/mman.h>
#include <unistd.h>

static void test_integer_gathers(void) {
    int64_t table64[64];
    int32_t table32[64];
    zmm_test_t index, initial64, out64;
    ymm_t initial32, out32;
    const int32_t indices32[8] = {-16, -1, 0, 1, 7, 15, 31, 47};
    const int64_t indices64[8] = {47, 31, 15, 7, 1, 0, -1, -16};
    const uint32_t mask = UINT32_C(0xa5);
    uint32_t mask_out = UINT32_MAX;

    for (int i = 0; i < 64; i++) {
        table64[i] = INT64_C(0x1234000000000000) + (int64_t)i * 0x101;
        table32[i] = (int32_t)(UINT32_C(0x81000000) + (uint32_t)i * 0x101U);
    }
    memcpy(index.i32, indices32, sizeof(indices32));
    for (int i = 0; i < 8; i++) initial64.i64[i] = -1000 - i;

    __asm__ volatile(
        "kmovb %5, %%k1\n\t"
        "vmovdqa64 %2, %%zmm0\n\t"
        "vmovdqa32 %3, %%ymm1\n\t"
        "vpgatherdq (%4,%%ymm1,8), %%zmm0%{%%k1%}\n\t"
        "vmovdqa64 %%zmm0, %0\n\t"
        "kmovb %%k1, %1"
        : "=m"(out64), "=r"(mask_out)
        : "m"(initial64), "m"(index), "r"(table64 + 16), "r"(mask)
        : "zmm0", "ymm1", "k1", "memory");
    for (int i = 0; i < 8; i++) {
        int64_t expected = (mask >> i) & 1U
            ? table64[16 + indices32[i]] : initial64.i64[i];
        TEST_ASSERT(out64.i64[i] == expected, "VPGATHERDQ lane %d", i);
    }
    TEST_ASSERT((mask_out & UINT32_C(0xff)) == 0,
                "VPGATHERDQ clears completed mask bits: 0x%02x", mask_out);

    memcpy(index.i64, indices64, sizeof(indices64));
    for (int i = 0; i < 8; i++) initial32.i32[i] = -2000 - i;
    mask_out = UINT32_MAX;
    __asm__ volatile(
        "kmovb %5, %%k1\n\t"
        "vmovdqa32 %2, %%ymm0\n\t"
        "vmovdqa64 %3, %%zmm1\n\t"
        "vpgatherqd (%4,%%zmm1,4), %%ymm0%{%%k1%}\n\t"
        "vmovdqa32 %%ymm0, %0\n\t"
        "kmovb %%k1, %1"
        : "=m"(out32), "=r"(mask_out)
        : "m"(initial32), "m"(index), "r"(table32 + 16), "r"(mask)
        : "ymm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 8; i++) {
        int32_t expected = (mask >> i) & 1U
            ? table32[16 + indices64[i]] : initial32.i32[i];
        TEST_ASSERT(out32.i32[i] == expected, "VPGATHERQD lane %d", i);
    }
    TEST_ASSERT((mask_out & UINT32_C(0xff)) == 0,
                "VPGATHERQD clears completed mask bits: 0x%02x", mask_out);
}

static void test_floating_gathers(void) {
    uint64_t table64[64];
    uint32_t table32[64];
    zmm_test_t index, initial64, out64;
    ymm_t initial32, out32;
    const int32_t indices32[8] = {0, 63, 1, 62, 7, 48, 16, 31};
    const int64_t indices64[8] = {63, 0, 62, 1, 48, 7, 31, 16};
    const uint32_t mask = UINT32_C(0x81);
    uint32_t mask_out = UINT32_MAX;

    for (int i = 0; i < 64; i++) {
        table64[i] = UINT64_C(0x3ff0000000000000) + (uint64_t)i;
        table32[i] = UINT32_C(0x3f800000) + (uint32_t)i;
    }
    memcpy(index.i32, indices32, sizeof(indices32));
    for (int i = 0; i < 8; i++) initial64.u64[i] = UINT64_C(0xdead000000000000) + (uint64_t)i;

    __asm__ volatile(
        "kmovb %5, %%k1\n\t"
        "vmovdqa64 %2, %%zmm0\n\t"
        "vmovdqa32 %3, %%ymm1\n\t"
        "vgatherdpd (%4,%%ymm1,8), %%zmm0%{%%k1%}\n\t"
        "vmovdqa64 %%zmm0, %0\n\t"
        "kmovb %%k1, %1"
        : "=m"(out64), "=r"(mask_out)
        : "m"(initial64), "m"(index), "r"(table64), "r"(mask)
        : "zmm0", "ymm1", "k1", "memory");
    for (int i = 0; i < 8; i++) {
        uint64_t expected = (mask >> i) & 1U ? table64[indices32[i]] : initial64.u64[i];
        TEST_ASSERT(out64.u64[i] == expected, "VGATHERDPD lane %d", i);
    }
    TEST_ASSERT((mask_out & UINT32_C(0xff)) == 0,
                "VGATHERDPD clears completed mask bits: 0x%02x", mask_out);

    memcpy(index.i64, indices64, sizeof(indices64));
    for (int i = 0; i < 8; i++) initial32.u32[i] = UINT32_C(0xdead0000) + (uint32_t)i;
    mask_out = UINT32_MAX;
    __asm__ volatile(
        "kmovb %5, %%k1\n\t"
        "vmovdqa32 %2, %%ymm0\n\t"
        "vmovdqa64 %3, %%zmm1\n\t"
        "vgatherqps (%4,%%zmm1,4), %%ymm0%{%%k1%}\n\t"
        "vmovdqa32 %%ymm0, %0\n\t"
        "kmovb %%k1, %1"
        : "=m"(out32), "=r"(mask_out)
        : "m"(initial32), "m"(index), "r"(table32), "r"(mask)
        : "ymm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 8; i++) {
        uint32_t expected = (mask >> i) & 1U ? table32[indices64[i]] : initial32.u32[i];
        TEST_ASSERT(out32.u32[i] == expected, "VGATHERQPS lane %d", i);
    }
    TEST_ASSERT((mask_out & UINT32_C(0xff)) == 0,
                "VGATHERQPS clears completed mask bits: 0x%02x", mask_out);
}

static void test_scatters(void) {
    int64_t out64[64];
    int32_t out32[64];
    uint64_t fp64[64];
    uint32_t fp32[64];
    zmm_test_t index, values64;
    ymm_t values32;
    const int32_t indices32[8] = {31, 0, 15, 1, 7, 48, 2, 63};
    const int64_t indices64[8] = {63, 2, 48, 7, 1, 15, 0, 31};
    const uint32_t mask = UINT32_C(0xa5);

    for (int i = 0; i < 64; i++) {
        out64[i] = INT64_MIN;
        out32[i] = INT32_MIN;
        fp64[i] = UINT64_MAX;
        fp32[i] = UINT32_MAX;
    }
    for (int i = 0; i < 8; i++) {
        index.i32[i] = indices32[i];
        values64.i64[i] = INT64_C(0x2345000000000000) + i;
    }
    __asm__ volatile(
        "kmovb %3, %%k1\n\t"
        "vmovdqa64 %0, %%zmm0\n\t"
        "vmovdqa32 %1, %%ymm1\n\t"
        "vpscatterdq %%zmm0, (%2,%%ymm1,8)%{%%k1%}"
        : : "m"(values64), "m"(index), "r"(out64), "r"(mask)
        : "zmm0", "ymm1", "k1", "memory");
    for (int pos = 0; pos < 64; pos++) {
        int lane = -1;
        for (int i = 0; i < 8; i++)
            if (((mask >> i) & 1U) && indices32[i] == pos) lane = i;
        int64_t expected = lane >= 0 ? values64.i64[lane] : INT64_MIN;
        TEST_ASSERT(out64[pos] == expected, "VPSCATTERDQ position %d", pos);
    }

    for (int i = 0; i < 8; i++) {
        index.i64[i] = indices64[i];
        values32.i32[i] = INT32_C(0x34560000) + i;
    }
    __asm__ volatile(
        "kmovb %3, %%k1\n\t"
        "vmovdqa32 %0, %%ymm0\n\t"
        "vmovdqa64 %1, %%zmm1\n\t"
        "vpscatterqd %%ymm0, (%2,%%zmm1,4)%{%%k1%}"
        : : "m"(values32), "m"(index), "r"(out32), "r"(mask)
        : "ymm0", "zmm1", "k1", "memory");
    for (int pos = 0; pos < 64; pos++) {
        int lane = -1;
        for (int i = 0; i < 8; i++)
            if (((mask >> i) & 1U) && indices64[i] == pos) lane = i;
        int32_t expected = lane >= 0 ? values32.i32[lane] : INT32_MIN;
        TEST_ASSERT(out32[pos] == expected, "VPSCATTERQD position %d", pos);
    }

    for (int i = 0; i < 8; i++) {
        index.i32[i] = indices32[i];
        values64.u64[i] = UINT64_C(0x4000000000000000) + (uint64_t)i;
    }
    __asm__ volatile(
        "kmovb %3, %%k1\n\t"
        "vmovdqa64 %0, %%zmm0\n\t"
        "vmovdqa32 %1, %%ymm1\n\t"
        "vscatterdpd %%zmm0, (%2,%%ymm1,8)%{%%k1%}"
        : : "m"(values64), "m"(index), "r"(fp64), "r"(mask)
        : "zmm0", "ymm1", "k1", "memory");
    for (int pos = 0; pos < 64; pos++) {
        int lane = -1;
        for (int i = 0; i < 8; i++)
            if (((mask >> i) & 1U) && indices32[i] == pos) lane = i;
        uint64_t expected = lane >= 0 ? values64.u64[lane] : UINT64_MAX;
        TEST_ASSERT(fp64[pos] == expected, "VSCATTERDPD position %d", pos);
    }

    for (int i = 0; i < 8; i++) {
        index.i64[i] = indices64[i];
        values32.u32[i] = UINT32_C(0x40000000) + (uint32_t)i;
    }
    __asm__ volatile(
        "kmovb %3, %%k1\n\t"
        "vmovdqa32 %0, %%ymm0\n\t"
        "vmovdqa64 %1, %%zmm1\n\t"
        "vscatterqps %%ymm0, (%2,%%zmm1,4)%{%%k1%}"
        : : "m"(values32), "m"(index), "r"(fp32), "r"(mask)
        : "ymm0", "zmm1", "k1", "memory");
    for (int pos = 0; pos < 64; pos++) {
        int lane = -1;
        for (int i = 0; i < 8; i++)
            if (((mask >> i) & 1U) && indices64[i] == pos) lane = i;
        uint32_t expected = lane >= 0 ? values32.u32[lane] : UINT32_MAX;
        TEST_ASSERT(fp32[pos] == expected, "VSCATTERQPS position %d", pos);
    }
}

static void test_masked_fault_suppression(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t size = (size_t)page_size * 2U;
    unsigned char *pages = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(pages != MAP_FAILED, "mixed gather/scatter guard allocation");
    if (pages == MAP_FAILED) return;
    TEST_ASSERT(mprotect(pages + page_size, (size_t)page_size, PROT_NONE) == 0,
                "mixed gather/scatter guard protection");

    zmm_test_t index, initial64, out64;
    ymm_t initial32, out32;
    for (int i = 0; i < 8; i++) {
        ((uint64_t *)pages)[i] = UINT64_C(0x5566000000000000) + (uint64_t)i;
        initial64.u64[i] = UINT64_C(0xaaaa000000000000) + (uint64_t)i;
        initial32.u32[i] = UINT32_C(0xbbbb0000) + (uint32_t)i;
        index.i32[i] = i;
    }
    index.i32[7] = (int32_t)(page_size / (long)sizeof(uint64_t));
    const uint32_t mask = UINT32_C(0x7f);
    __asm__ volatile(
        "kmovb %4, %%k1\n\tvmovdqa64 %2, %%zmm0\n\tvmovdqa32 %3, %%ymm1\n\t"
        "vpgatherdq (%1,%%ymm1,8), %%zmm0%{%%k1%}\n\tvmovdqa64 %%zmm0, %0\n\t"
        "kmovb %4, %%k1\n\tvpscatterdq %%zmm0, (%1,%%ymm1,8)%{%%k1%}"
        : "=m"(out64)
        : "r"(pages), "m"(initial64), "m"(index), "r"(mask)
        : "zmm0", "ymm1", "k1", "memory");
    for (int i = 0; i < 7; i++)
        TEST_ASSERT(out64.u64[i] == ((uint64_t *)pages)[i],
                    "VPGATHERDQ active guard lane %d", i);
    TEST_ASSERT(out64.u64[7] == initial64.u64[7],
                "VPGATHERDQ suppresses masked guard lane");

    for (int i = 0; i < 8; i++) index.i64[i] = i;
    index.i64[7] = page_size / (long)sizeof(uint32_t);
    __asm__ volatile(
        "kmovb %4, %%k1\n\tvmovdqa32 %2, %%ymm0\n\tvmovdqa64 %3, %%zmm1\n\t"
        "vpgatherqd (%1,%%zmm1,4), %%ymm0%{%%k1%}\n\tvmovdqa32 %%ymm0, %0\n\t"
        "kmovb %4, %%k1\n\tvpscatterqd %%ymm0, (%1,%%zmm1,4)%{%%k1%}"
        : "=m"(out32)
        : "r"(pages), "m"(initial32), "m"(index), "r"(mask)
        : "ymm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 7; i++)
        TEST_ASSERT(out32.u32[i] == ((uint32_t *)pages)[i],
                    "VPGATHERQD active guard lane %d", i);
    TEST_ASSERT(out32.u32[7] == initial32.u32[7],
                "VPGATHERQD suppresses masked guard lane");

    munmap(pages, size);
}

int main(void) {
    TEST_START("AVX-512 mixed-width gather/scatter forms");
    if (!CHECK_AVX512F()) return 0;
    test_integer_gathers();
    test_floating_gathers();
    test_scatters();
    test_masked_fault_suppression();
    TEST_END();
}
