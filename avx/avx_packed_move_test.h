#ifndef X86_64_AVX_PACKED_MOVE_TEST_H
#define X86_64_AVX_PACKED_MOVE_TEST_H

#include "../common.h"

int main(void) {
    ymm_t src, out, initial;
    TEST_START(TEST_NAME);
    for (int i = 0; i < 32; i++) src.u8[i] = (uint8_t)(i * 7 + 3);
#if TEST_ALIGNED
    __asm__ volatile(TEST_INSN " %1, %%ymm0\n\t" TEST_INSN " %%ymm0, %0"
                     : "=m"(out) : "m"(src) : "ymm0", "memory");
#else
    uint8_t source_storage[33], dest_storage[33];
    memcpy(source_storage + 1, &src, sizeof(src));
    memset(dest_storage, 0, sizeof(dest_storage));
    __asm__ volatile(TEST_INSN " %1, %%ymm0\n\t" TEST_INSN " %%ymm0, %0"
                     : "=m"(*(uint8_t (*)[32])(dest_storage + 1))
                     : "m"(*(const uint8_t (*)[32])(source_storage + 1))
                     : "ymm0", "memory");
    memcpy(&out, dest_storage + 1, sizeof(out));
#endif
    TEST_ASSERT(memcmp(&out, &src, sizeof(src)) == 0, TEST_NAME " 256-bit load/store");
    memset(&initial, 0xa5, sizeof(initial));
    __asm__ volatile("vmovdqu %1, %%ymm0\n\t" TEST_INSN " %2, %%xmm0\n\t"
                     "vmovdqu %%ymm0, %0"
                     : "=m"(out) : "m"(initial), "m"(src) : "ymm0");
    TEST_ASSERT(memcmp(&out, &src, 16) == 0, TEST_NAME " 128-bit low data");
    TEST_ASSERT(out.u64[2] == 0 && out.u64[3] == 0, TEST_NAME " VEX.128 upper zero");
    TEST_END();
}

#endif
