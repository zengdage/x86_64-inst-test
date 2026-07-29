#ifndef X86_64_TEST_COMMON_H
#define X86_64_TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(cond, fmt, ...) \
    do { \
        if (cond) { \
            test_passed++; \
        } else { \
            test_failed++; \
            printf("  FAIL: " fmt "\n", ##__VA_ARGS__); \
        } \
    } while (0)

#define TEST_START(name) \
    printf("=== Testing %s ===\n", name)

#define TEST_END() \
    do { \
        printf("Results: %d passed, %d failed\n\n", test_passed, test_failed); \
        return test_failed ? 1 : 0; \
    } while (0)

static inline uint64_t get_flags(void) {
    uint64_t flags;
    __asm__ volatile (
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
    );
    return flags;
}

#define CF_FLAG  (1UL << 0)
#define PF_FLAG  (1UL << 2)
#define AF_FLAG  (1UL << 4)
#define ZF_FLAG  (1UL << 6)
#define SF_FLAG  (1UL << 7)
#define OF_FLAG  (1UL << 11)

typedef union {
    __int128 i128;
    int64_t  i64[2];
    int32_t  i32[4];
    int16_t  i16[8];
    int8_t   i8[16];
    uint64_t u64[2];
    uint32_t u32[4];
    uint16_t u16[8];
    uint8_t  u8[16];
    float    f32[4];
    double   f64[2];
} xmm_t __attribute__((aligned(16)));

typedef union {
    int64_t  i64[4];
    int32_t  i32[8];
    int16_t  i16[16];
    int8_t   i8[32];
    uint64_t u64[4];
    uint32_t u32[8];
    uint16_t u16[16];
    uint8_t  u8[32];
    float    f32[8];
    double   f64[4];
} ymm_t __attribute__((aligned(32)));

#endif
