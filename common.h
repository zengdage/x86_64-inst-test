#ifndef X86_64_TEST_COMMON_H
#define X86_64_TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

/*
 * Instruction tests execute directly by default.  Define this to 1 when a
 * native run should probe CPUID first and skip unsupported instruction sets.
 */
#ifndef ENABLE_RUNTIME_CPU_CHECKS
#define ENABLE_RUNTIME_CPU_CHECKS 0
#endif

#ifndef ENABLE_MXCSR_CHECK
/* MXCSR state/exception checks are opt-in for native and translator runs. */
#define ENABLE_MXCSR_CHECK 0
#endif

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

static inline int is_qnan_f32(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & UINT32_C(0x7fffffff)) >= UINT32_C(0x7fc00000);
}

static inline int is_snan_f32(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & UINT32_C(0x7fc00000)) == UINT32_C(0x7f800000) &&
           (bits & UINT32_C(0x003fffff)) != 0;
}

static inline int is_qnan_f64(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & UINT64_C(0x7fffffffffffffff)) >=
           UINT64_C(0x7ff8000000000000);
}

static inline int is_snan_f64(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & UINT64_C(0x7ff8000000000000)) ==
               UINT64_C(0x7ff0000000000000) &&
           (bits & UINT64_C(0x0007ffffffffffff)) != 0;
}

#define IS_QNAN(value) _Generic((value), \
    float: is_qnan_f32, \
    double: is_qnan_f64 \
)(value)

#define IS_SNAN(value) _Generic((value), \
    float: is_snan_f32, \
    double: is_snan_f64 \
)(value)

#endif
