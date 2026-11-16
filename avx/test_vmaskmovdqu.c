/* TARGET: VMASKMOVDQU */
#include "../common.h"
#include <sys/mman.h>
#include <unistd.h>

static void run_store(uint8_t *dst, const uint8_t *data, const uint8_t *mask,
                      uintptr_t *rdi_after) {
    __asm__ volatile("vmovdqu %2, %%xmm0\n\tvmovdqu %3, %%xmm1\n\t"
                     "mov %1, %%rdi\n\tvmaskmovdqu %%xmm1, %%xmm0\n\t"
                     "mov %%rdi, %0"
                     : "=&r"(*rdi_after)
                     : "r"(dst), "m"(*(const uint8_t (*)[16])data),
                       "m"(*(const uint8_t (*)[16])mask)
                     : "xmm0", "xmm1", "rdi", "memory");
}

int main(void) {
    uint8_t data[16], mask[16], dst[16];
    uintptr_t rdi_after;
    long page_size;
    uint8_t *pages;
    TEST_START("VMASKMOVDQU");
    for (int i = 0; i < 16; i++) {
        data[i] = (uint8_t)(0x40 + i);
        mask[i] = (uint8_t)((i == 0 || i == 15 || i % 3 == 0) ? 0x80 : 0x7f);
        dst[i] = 0xa5;
    }
    run_store(dst, data, mask, &rdi_after);
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst[i] == ((mask[i] & 0x80) ? data[i] : 0xa5),
                    "VMASKMOVDQU sign mask byte %d", i);
    TEST_ASSERT(rdi_after == (uintptr_t)dst, "VMASKMOVDQU preserves RDI");

    page_size = sysconf(_SC_PAGESIZE);
    TEST_ASSERT(page_size > 0, "VMASKMOVDQU page size");
    if (page_size <= 0) return 1;
    pages = mmap(NULL, (size_t)page_size * 2, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(pages != MAP_FAILED, "VMASKMOVDQU pages");
    if (pages == MAP_FAILED) return 1;
    TEST_ASSERT(mprotect(pages + page_size, (size_t)page_size, PROT_NONE) == 0,
                "VMASKMOVDQU guard page");
    memset(mask, 0x80, sizeof(mask));
    run_store(pages + page_size - 16, data, mask, &rdi_after);
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(pages[page_size - 16 + i] == data[i],
                    "VMASKMOVDQU boundary byte %d", i);
    munmap(pages, (size_t)page_size * 2);
    TEST_END();
}
