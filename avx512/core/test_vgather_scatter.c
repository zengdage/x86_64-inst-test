/* AVX-512 gather/scatter data, mask update, scales, and fault suppression. */
#include "../include/avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

static void test_gather_dword(void) {
    int32_t table[64];
    zmm_test_t index, initial, out;
    for (int i = 0; i < 64; i++) table[i] = 1000 + i * 17;
    const int32_t indices[16] = {15, 0, 31, 1, 7, 7, 2, 30, 4, 29, 8, 28, 16, 3, 27, 63};
    memcpy(index.i32, indices, sizeof(indices));
    for (int i = 0; i < 16; i++) initial.i32[i] = -1 - i;
    uint32_t mask_in = 0x8429U;
    uint32_t mask_out;
    __asm__ volatile(
        "kmovw %5, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
        "vmovdqa32 %3, %%zmm1\n\t"
        "vpgatherdd (%4,%%zmm1,4), %%zmm0%{%%k1%}\n\t"
        "vmovdqa32 %%zmm0, %0\n\tkmovw %%k1, %1"
        : "=m"(out), "=r"(mask_out)
        : "m"(initial), "m"(index), "r"(table), "r"(mask_in)
        : "zmm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 16; i++) {
        int32_t expected = ((mask_in >> i) & 1U) ? table[index.i32[i]] : initial.i32[i];
        TEST_ASSERT(out.i32[i] == expected, "VPGATHERDD lane %d", i);
    }
    TEST_ASSERT((mask_out & 0xffffU) == 0, "VPGATHERDD clears successful mask bits");

    /* Scale 1 with byte offsets and full mask. */
    for (int i = 0; i < 16; i++) index.i32[i] = i * 4;
    mask_in = 0xffffU;
    __asm__ volatile(
        "kmovw %4, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
        "vmovdqa32 %1, %%zmm1\n\t"
        "vpgatherdd (%3,%%zmm1,1), %%zmm0%{%%k1%}\n\t"
        "vmovdqa32 %%zmm0, %0"
        : "=m"(out) : "m"(index), "m"(initial), "r"(table), "r"(mask_in)
        : "zmm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(out.i32[i] == table[i], "VPGATHERDD scale=1 lane %d", i);
}

static void test_gather_qword_and_scatter(void) {
    int64_t table[32];
    zmm_test_t index, initial, out, values;
    for (int i = 0; i < 32; i++) table[i] = INT64_C(0x100000000) + i;
    const int64_t indices[8] = {31, 0, 15, 1, 7, 7, 2, 30};
    memcpy(index.i64, indices, sizeof(indices));
    for (int i = 0; i < 8; i++) initial.i64[i] = -100 - i;
    uint32_t mask = 0xa5U;
    __asm__ volatile(
        "kmovb %4, %%k1\n\tvmovdqa64 %2, %%zmm0\n\t"
        "vmovdqa64 %1, %%zmm1\n\t"
        "vpgatherqq (%3,%%zmm1,8), %%zmm0%{%%k1%}\n\t"
        "vmovdqa64 %%zmm0, %0"
        : "=m"(out) : "m"(index), "m"(initial), "r"(table), "r"(mask)
        : "zmm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 8; i++) {
        int64_t expected = ((mask >> i) & 1U) ? table[index.i64[i]] : initial.i64[i];
        TEST_ASSERT(out.i64[i] == expected, "VPGATHERQQ lane %d", i);
    }

    int32_t scattered[32];
    for (int i = 0; i < 32; i++) scattered[i] = -1;
    for (int i = 0; i < 16; i++) {
        index.i32[i] = (i * 7) & 31;
        values.i32[i] = 5000 + i;
    }
    uint32_t mask16 = 0x8429U;
    __asm__ volatile(
        "kmovw %3, %%k1\n\tvmovdqa32 %0, %%zmm0\n\t"
        "vmovdqa32 %1, %%zmm1\n\t"
        "vpscatterdd %%zmm0, (%2,%%zmm1,4)%{%%k1%}"
        : : "m"(values), "m"(index), "r"(scattered), "r"(mask16)
        : "zmm0", "zmm1", "k1", "memory");
    for (int i = 0; i < 16; i++) {
        if ((mask16 >> i) & 1U)
            TEST_ASSERT(scattered[index.i32[i]] == values.i32[i],
                        "VPSCATTERDD selected lane %d", i);
    }
    for (int pos = 0; pos < 32; pos++) {
        int selected = 0;
        for (int i = 0; i < 16; i++)
            selected |= ((mask16 >> i) & 1U) && index.i32[i] == pos;
        if (!selected)
            TEST_ASSERT(scattered[pos] == -1, "VPSCATTERDD leaves position %d", pos);
    }
}

static void test_empty_mask_fault_suppression(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "gather/scatter guard page allocation");
    if (guard == MAP_FAILED) return;

    zmm_test_t index = {0}, initial, out;
    for (int i = 0; i < 16; i++) initial.u32[i] = 0x12340000U + (uint32_t)i;
    uint32_t zero = 0;
    __asm__ volatile(
        "kmovw %4, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
        "vmovdqa32 %3, %%zmm1\n\t"
        "vpgatherdd (%1,%%zmm1,4), %%zmm0%{%%k1%}\n\t"
        "vmovdqa32 %%zmm0, %0\n\t"
        "kmovw %4, %%k1\n\t"
        "vpscatterdd %%zmm0, (%1,%%zmm1,4)%{%%k1%}"
        : "=m"(out)
        : "r"(guard), "m"(initial), "m"(index), "r"(zero)
        : "zmm0", "zmm1", "k1", "memory");
    TEST_ASSERT(memcmp(&out, &initial, sizeof(out)) == 0,
                "empty gather/scatter masks suppress invalid addresses");
    munmap(guard, (size_t)page_size);
}

int main(void) {
    TEST_START("AVX-512 VPGATHER/VPSCATTER");
    if (!CHECK_AVX512F()) return 0;
    test_gather_dword();
    test_gather_qword_and_scatter();
    test_empty_mask_fault_suppression();
    TEST_END();
}
