/* AVX-512 compress/expand register and memory forms with fault suppression. */
#include "avx512_test.h"
#include <sys/mman.h>
#include <unistd.h>

static int selected(unsigned mask, int lane) {
    return (mask >> lane) & 1U;
}

static void test_dword_register_forms(void) {
    zmm_test_t src, merge, compressed, expanded;
    for (int i = 0; i < 16; i++) {
        src.u32[i] = 100U + (uint32_t)i;
        merge.u32[i] = 0xdead0000U + (uint32_t)i;
    }
    const uint32_t mask = 0x8429U; /* lanes 0,3,5,10,15 */
    __asm__ volatile(
        "kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
        "vmovdqa32 %2, %%zmm1\n\t"
        "vpcompressd %%zmm0, %%zmm1%{%%k1%}\n\t"
        "vmovdqa32 %%zmm1, %0"
        : "=m"(compressed) : "m"(src), "m"(merge), "r"(mask)
        : "zmm0", "zmm1", "k1");
    int packed = 0;
    int selected_count = 0;
    for (int i = 0; i < 16; i++) selected_count += selected(mask, i);
    for (int i = 0; i < 16; i++) {
        while (packed < 16 && !selected(mask, packed)) packed++;
        uint32_t expected = i < selected_count ? src.u32[packed++] : merge.u32[i];
        TEST_ASSERT(compressed.u32[i] == expected,
                    "VPCOMPRESSD merge lane %d: 0x%x != 0x%x",
                    i, compressed.u32[i], expected);
    }

    __asm__ volatile(
        "kmovw %3, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
        "vmovdqa32 %2, %%zmm1\n\t"
        "vpexpandd %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovdqa32 %%zmm1, %0"
        : "=m"(expanded) : "m"(compressed), "m"(merge), "r"(mask)
        : "zmm0", "zmm1", "k1");
    packed = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t expected = selected(mask, i) ? compressed.u32[packed++] : 0;
        TEST_ASSERT(expanded.u32[i] == expected,
                    "VPEXPANDD zero lane %d: 0x%x != 0x%x",
                    i, expanded.u32[i], expected);
    }

    /* Source and destination overlap is architectural for both instructions. */
    __asm__ volatile(
        "kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
        "vpcompressd %%zmm0, %%zmm0%{%%k1%}%{z%}\n\t"
        "vmovdqa32 %%zmm0, %0"
        : "=m"(compressed) : "m"(src), "r"(mask) : "zmm0", "k1");
    packed = 0;
    selected_count = 0;
    for (int i = 0; i < 16; i++) selected_count += selected(mask, i);
    for (int i = 0; i < 16; i++) {
        while (packed < 16 && !selected(mask, packed)) packed++;
        uint32_t expected = i < selected_count ? src.u32[packed++] : 0;
        TEST_ASSERT(compressed.u32[i] == expected,
                    "VPCOMPRESSD overlapping operands lane %d", i);
    }
}

static void test_qword_and_float_forms(void) {
    zmm_test_t src, out;
    for (int i = 0; i < 8; i++) src.u64[i] = UINT64_C(0x100000000) + (uint64_t)i;
    uint32_t mask8 = 0xa5U;
    __asm__ volatile(
        "kmovb %2, %%k1\n\tvmovdqa64 %1, %%zmm0\n\t"
        "vpcompressq %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovdqa64 %%zmm1, %0"
        : "=m"(out) : "m"(src), "r"(mask8) : "zmm0", "zmm1", "k1");
    int packed = 0;
    int selected_count = 0;
    for (int i = 0; i < 8; i++) selected_count += selected(mask8, i);
    for (int i = 0; i < 8; i++) {
        while (packed < 8 && !selected(mask8, packed)) packed++;
        uint64_t expected = i < selected_count ? src.u64[packed++] : 0;
        TEST_ASSERT(out.u64[i] == expected, "VPCOMPRESSQ lane %d", i);
    }

    for (int i = 0; i < 16; i++) src.u32[i] = UINT32_C(0x7fc00000) + (uint32_t)i;
    uint32_t mask16 = 0x8001U;
    __asm__ volatile(
        "kmovw %2, %%k1\n\tvmovaps %1, %%zmm0\n\t"
        "vcompressps %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovaps %%zmm1, %0"
        : "=m"(out) : "m"(src), "r"(mask16) : "zmm0", "zmm1", "k1");
    TEST_ASSERT(out.u32[0] == src.u32[0] && out.u32[1] == src.u32[15],
                "VCOMPRESSPS preserves NaN payload bits and order");
    for (int i = 2; i < 16; i++)
        TEST_ASSERT(out.u32[i] == 0, "VCOMPRESSPS zero mask lane %d", i);

    __asm__ volatile(
        "kmovb %2, %%k1\n\tvmovapd %1, %%zmm0\n\t"
        "vexpandpd %%zmm0, %%zmm1%{%%k1%}%{z%}\n\t"
        "vmovapd %%zmm1, %0"
        : "=m"(out) : "m"(src), "r"(mask8) : "zmm0", "zmm1", "k1");
    packed = 0;
    for (int i = 0; i < 8; i++) {
        uint64_t expected = selected(mask8, i) ? src.u64[packed++] : 0;
        TEST_ASSERT(out.u64[i] == expected, "VEXPANDPD bit-exact lane %d", i);
    }
}

static void test_memory_forms_and_fault_suppression(void) {
    zmm_test_t src, out;
    uint32_t memory[16];
    for (int i = 0; i < 16; i++) {
        src.u32[i] = 0xabc00000U + (uint32_t)i;
        memory[i] = 0xeeeeeeeeU;
    }
    uint32_t mask = 0x8429U;
    static const int selected_lanes[5] = {0, 3, 5, 10, 15};
    __asm__ volatile(
        "kmovw %2, %%k1\n\tvmovdqa32 %1, %%zmm0\n\t"
        "vpcompressd %%zmm0, %0%{%%k1%}"
        : "+m"(memory) : "m"(src), "r"(mask) : "zmm0", "k1", "memory");
    int packed = 0;
    for (int i = 0; i < 5; i++)
        TEST_ASSERT(memory[i] == src.u32[selected_lanes[i]],
                    "VPCOMPRESSD memory element %d", i);
    for (int i = 5; i < 16; i++)
        TEST_ASSERT(memory[i] == 0xeeeeeeeeU, "VPCOMPRESSD exact memory footprint %d", i);

    memset(&out, 0xcc, sizeof(out));
    __asm__ volatile(
        "kmovw %3, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
        "vpexpandd %1, %%zmm0%{%%k1%}\n\tvmovdqa32 %%zmm0, %0"
        : "=m"(out) : "m"(memory), "m"(out), "r"(mask)
        : "zmm0", "k1", "memory");
    packed = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t expected = selected(mask, i) ? memory[packed++] : UINT32_C(0xcccccccc);
        TEST_ASSERT(out.u32[i] == expected, "VPEXPANDD memory merge lane %d", i);
    }

    long page_size = sysconf(_SC_PAGESIZE);
    void *guard = mmap(NULL, (size_t)page_size, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(guard != MAP_FAILED, "compress/expand guard page allocation");
    if (guard != MAP_FAILED) {
        uint32_t zero = 0;
        __asm__ volatile(
            "kmovw %1, %%k1\n\tvmovdqa32 %2, %%zmm0\n\t"
            "vpcompressd %%zmm0, (%0)%{%%k1%}\n\t"
            "vpexpandd (%0), %%zmm0%{%%k1%}"
            : : "r"(guard), "r"(zero), "m"(src)
            : "zmm0", "k1", "memory");
        TEST_ASSERT(1, "empty masks suppress invalid compress/expand memory access");
        munmap(guard, (size_t)page_size);
    }
}

int main(void) {
    TEST_START("VPCOMPRESS/VPEXPAND and VCOMPRESS/VEXPAND");
    if (!CHECK_AVX512F()) return 0;
    test_dword_register_forms();
    test_qword_and_float_forms();
    test_memory_forms_and_fault_suppression();
    TEST_END();
}
