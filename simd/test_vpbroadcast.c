/*
 * test_vpbroadcast.c - Test AVX2 VPBROADCASTB/W/D/Q instructions.
 *
 * Each instruction is exercised with memory and XMM register sources, using
 * both VEX.128 (XMM destination) and VEX.256 (YMM destination) encodings.
 */
#include "../common.h"

/*
 * The VEX.128 cases begin with non-zero YMM state and save the complete
 * register after the broadcast.  This makes the architectural upper-lane
 * zeroing visible instead of only checking the XMM result.
 */
#define TEST_VPBROADCAST(MNEM, TYPE, FIELD, LANES128, LANES256, VALUE)          \
    do {                                                                        \
        TYPE mem_value = (TYPE)(VALUE);                                         \
        ymm_t seed;                                                             \
        ymm_t result;                                                           \
        size_t i;                                                               \
                                                                                \
        memset(&seed, 0x5a, sizeof(seed));                                      \
        seed.FIELD[0] = (TYPE)(VALUE);                                          \
                                                                                \
        /* Memory source, VEX.128 destination: upper YMM half is cleared. */   \
        __asm__ volatile (                                                      \
            "vmovdqu %1, %%ymm0\n\t"                                           \
            #MNEM " %2, %%xmm0\n\t"                                            \
            "vmovdqu %%ymm0, %0"                                                \
            : "=m"(result) : "m"(seed), "m"(mem_value) : "ymm0");            \
        for (i = 0; i < (LANES128); i++)                                        \
            TEST_ASSERT(result.FIELD[i] == (TYPE)(VALUE),                      \
                        #MNEM " memory VEX.128 lane %zu: got %#" PRIx64       \
                        " expected %#" PRIx64,                                 \
                        i, (uint64_t)result.FIELD[i],                          \
                        (uint64_t)(TYPE)(VALUE));                               \
        for (i = (LANES128); i < (LANES256); i++)                               \
            TEST_ASSERT(result.FIELD[i] == 0,                                   \
                        #MNEM " memory VEX.128 upper lane %zu not zero: %#"    \
                        PRIx64, i, (uint64_t)result.FIELD[i]);                  \
                                                                                \
        /* Memory source, VEX.256 destination. */                                \
        __asm__ volatile (                                                      \
            #MNEM " %1, %%ymm0\n\t"                                           \
            "vmovdqu %%ymm0, %0"                                                \
            : "=m"(result) : "m"(mem_value) : "ymm0");                        \
        for (i = 0; i < (LANES256); i++)                                        \
            TEST_ASSERT(result.FIELD[i] == (TYPE)(VALUE),                      \
                        #MNEM " memory VEX.256 lane %zu: got %#" PRIx64       \
                        " expected %#" PRIx64,                                 \
                        i, (uint64_t)result.FIELD[i],                          \
                        (uint64_t)(TYPE)(VALUE));                               \
                                                                                \
        /* Register source and same XMM/YMM register, VEX.128 destination. */   \
        __asm__ volatile (                                                      \
            "vmovdqu %1, %%ymm0\n\t"                                           \
            #MNEM " %%xmm0, %%xmm0\n\t"                                       \
            "vmovdqu %%ymm0, %0"                                                \
            : "=m"(result) : "m"(seed) : "ymm0");                              \
        for (i = 0; i < (LANES128); i++)                                        \
            TEST_ASSERT(result.FIELD[i] == (TYPE)(VALUE),                      \
                        #MNEM " register VEX.128 lane %zu: got %#" PRIx64     \
                        " expected %#" PRIx64,                                 \
                        i, (uint64_t)result.FIELD[i],                          \
                        (uint64_t)(TYPE)(VALUE));                               \
        for (i = (LANES128); i < (LANES256); i++)                               \
            TEST_ASSERT(result.FIELD[i] == 0,                                   \
                        #MNEM " register VEX.128 upper lane %zu not zero: %#"  \
                        PRIx64, i, (uint64_t)result.FIELD[i]);                  \
                                                                                \
        /* Register source and same architectural register, VEX.256. */        \
        __asm__ volatile (                                                      \
            "vmovdqu %1, %%ymm0\n\t"                                           \
            #MNEM " %%xmm0, %%ymm0\n\t"                                       \
            "vmovdqu %%ymm0, %0"                                                \
            : "=m"(result) : "m"(seed) : "ymm0");                              \
        for (i = 0; i < (LANES256); i++)                                        \
            TEST_ASSERT(result.FIELD[i] == (TYPE)(VALUE),                      \
                        #MNEM " register VEX.256 lane %zu: got %#" PRIx64     \
                        " expected %#" PRIx64,                                 \
                        i, (uint64_t)result.FIELD[i],                          \
                        (uint64_t)(TYPE)(VALUE));                               \
    } while (0)

int main(void) {
    TEST_START("VPBROADCASTB/W/D/Q instructions (AVX2)");

    TEST_VPBROADCAST(vpbroadcastb, uint8_t,  u8,  16, 32, UINT8_C(0xa5));
    TEST_VPBROADCAST(vpbroadcastw, uint16_t, u16, 8, 16, UINT16_C(0xbeef));
    TEST_VPBROADCAST(vpbroadcastd, uint32_t, u32, 4, 8, UINT32_C(0x89abcdef));
    TEST_VPBROADCAST(vpbroadcastq, uint64_t, u64, 2, 4,
                     UINT64_C(0x0123456789abcdef));

    __asm__ volatile ("vzeroupper");
    TEST_END();
}

