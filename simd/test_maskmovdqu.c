/*
 * Test MASKMOVDQU (SSE2).
 * The mask controls byte stores by its sign bits and the destination address
 * is taken implicitly from RDI.
 *
 * Compile: gcc -o test_maskmovdqu simd/test_maskmovdqu.c -O0 -msse2
 * Do NOT use static linking.
 */
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../common.h"

static void maskmovdqu_store(uint8_t *dst, const uint8_t *data,
                             const uint8_t *mask, uintptr_t *rdi_after) {
    __asm__ volatile (
        "movdqu %2, %%xmm0\n\t"
        "movdqu %3, %%xmm1\n\t"
        "mov %1, %%rdi\n\t"
        "maskmovdqu %%xmm1, %%xmm0\n\t"
        "mov %%rdi, %0"
        : "=&r"(*rdi_after)
        : "r"(dst), "m"(*(const uint8_t (*)[16])data),
          "m"(*(const uint8_t (*)[16])mask)
        : "xmm0", "xmm1", "rdi", "memory"
    );
}

static void test_mask_sign_bits_and_rdi(void) {
    const uint8_t data[16] = {
        0x10, 0x81, 0x22, 0x83, 0x44, 0x85, 0x66, 0x87,
        0x88, 0x09, 0xaa, 0x0b, 0xcc, 0x0d, 0xee, 0x0f
    };
    const uint8_t mask[16] = {
        0x80, 0x7f, 0x00, 0xff, 0x01, 0x81, 0x40, 0xc0,
        0x7e, 0x82, 0x02, 0xfe, 0x7f, 0x80, 0x03, 0xff
    };
    uint8_t dst[16];
    uintptr_t rdi_after;

    memset(dst, 0x5a, sizeof(dst));
    maskmovdqu_store(dst, data, mask, &rdi_after);
    for (int i = 0; i < 16; i++) {
        uint8_t expected = mask[i] & 0x80 ? data[i] : 0x5a;
        TEST_ASSERT(dst[i] == expected,
                    "MASKMOVDQU sign-bit mask byte %d: got %#x", i, dst[i]);
    }
    TEST_ASSERT(rdi_after == (uintptr_t)dst,
                "MASKMOVDQU preserves implicit RDI destination address");
}

static void test_data_mask_register_alias(void) {
    const uint8_t data_and_mask[16] = {
        0x00, 0x80, 0x7f, 0xff, 0x01, 0x81, 0x40, 0xc0,
        0x7e, 0x82, 0x02, 0xfe, 0x03, 0x83, 0x04, 0x84
    };
    uint8_t dst[16];
    uintptr_t rdi_after;

    memset(dst, 0xa5, sizeof(dst));
    __asm__ volatile (
        "movdqu %2, %%xmm0\n\t"
        "mov %1, %%rdi\n\t"
        "maskmovdqu %%xmm0, %%xmm0\n\t"
        "mov %%rdi, %0"
        : "=&r"(rdi_after)
        : "r"(dst), "m"(data_and_mask)
        : "xmm0", "rdi", "memory"
    );
    for (int i = 0; i < 16; i++) {
        uint8_t expected = data_and_mask[i] & 0x80 ? data_and_mask[i] : 0xa5;
        TEST_ASSERT(dst[i] == expected,
                    "MASKMOVDQU data=mask register byte %d: got %#x", i, dst[i]);
    }
    TEST_ASSERT(rdi_after == (uintptr_t)dst,
                "MASKMOVDQU data=mask preserves RDI");
}

static void test_store_at_page_boundary(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    TEST_ASSERT(page_size > 0, "MASKMOVDQU page size available");
    if (page_size <= 0)
        return;

    uint8_t *pages = mmap(NULL, (size_t)page_size * 2, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(pages != MAP_FAILED, "MASKMOVDQU guard page allocation");
    if (pages == MAP_FAILED)
        return;

    int protect_result = mprotect(pages + page_size, (size_t)page_size, PROT_NONE);
    TEST_ASSERT(protect_result == 0, "MASKMOVDQU guard page protection");
    if (protect_result == 0) {
        const uint8_t data[16] = {
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
        };
        const uint8_t all_mask[16] = {
            0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
        };
        uint8_t *boundary = pages + page_size - 16;
        uintptr_t rdi_after;

        memset(boundary, 0x5a, 16);
        maskmovdqu_store(boundary, data, all_mask, &rdi_after);
        for (int i = 0; i < 16; i++)
            TEST_ASSERT(boundary[i] == data[i],
                        "MASKMOVDQU final valid byte range %d", i);
        TEST_ASSERT(rdi_after == (uintptr_t)boundary,
                    "MASKMOVDQU boundary store preserves RDI");
    }
    munmap(pages, (size_t)page_size * 2);
}

int main(void) {
    TEST_START("MASKMOVDQU");
    test_mask_sign_bits_and_rdi();
    test_data_mask_register_alias();
    test_store_at_page_boundary();
    TEST_END();
}
